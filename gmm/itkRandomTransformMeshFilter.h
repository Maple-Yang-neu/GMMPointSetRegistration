#pragma once

#include <itkMeshToMeshFilter.h>
#include <itkTransform.h>
#include <itkIdentityTransform.h>
#include <itkNormalVariateGenerator.h>
#include <itkMersenneTwisterRandomVariateGenerator.h>

namespace itk
{
  template< typename TInputMesh, typename TOutputMesh = TInputMesh>
  class RandomTransformMeshFilter : public itk::MeshToMeshFilter< TInputMesh, TOutputMesh >
  {
  public:
    /** Standard class typedefs. */
    typedef RandomTransformMeshFilter                    Self;
    typedef MeshToMeshFilter< TInputMesh, TOutputMesh >  Superclass;
    typedef SmartPointer< Self >                         Pointer;
    typedef SmartPointer< const Self >                   ConstPointer;

    itkStaticConstMacro(Dimension, unsigned int, TInputMesh::PointDimension);

#ifdef ITK_USE_CONCEPT_CHECKING
    // Begin concept checking
    itkConceptMacro(SameDimensionCheck, (itk::Concept::SameDimension< TInputMesh::PointDimension, TOutputMesh::PointDimension >));
    // End concept checking
#endif

    typedef TInputMesh                                    InputMeshType;
    typedef typename InputMeshType::ConstPointer          InputMeshPointer;
    typedef typename InputMeshType::PointsContainer       InputPointsContainer;
    typedef typename InputPointsContainer::ConstPointer   InputPointsContainerConstPointer;

    typedef TOutputMesh                                   OutputMeshType;
    typedef typename OutputMeshType::Pointer              OutputMeshPointer;
    typedef typename OutputMeshType::PointsContainer      OutputPointsContainer;
    typedef typename OutputPointsContainer::Pointer       OutputPointsContainerPointer;

    typedef itk::Transform<double, Dimension, Dimension>  TransformType;
    typedef typename TransformType::ParametersType        ParametersType;

    /** Type of the transform. */
    typedef itk::Statistics::MersenneTwisterRandomVariateGenerator TransformGeneratorType;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(RandomTransformMeshFilter, MeshToMeshFilter);

    /** Get/Set transform. */
    itkSetObjectMacro(Transform, TransformType);
    itkGetModifiableObjectMacro(Transform, TransformType);

    itkSetObjectMacro(TransformGenerator, TransformGeneratorType);
    itkGetObjectMacro(TransformGenerator, TransformGeneratorType);

    itkSetMacro(StandardDeviation, double);
    itkGetMacro(StandardDeviation, double);

    itkSetMacro(RandomSeed, int);
    itkGetMacro(RandomSeed, int);

    itkSetMacro(LowerBounds, ParametersType);
    itkGetMacro(LowerBounds, ParametersType);

    itkSetMacro(UpperBounds, ParametersType);
    itkGetMacro(UpperBounds, ParametersType);

  protected:
    RandomTransformMeshFilter() 
    {
      m_Transform = ITK_NULLPTR;
      m_TransformGenerator = ITK_NULLPTR;
    };

    ~RandomTransformMeshFilter() {};

    /** Generate Requested Data */
    virtual void GenerateData() ITK_OVERRIDE
    {
      InputMeshPointer inputMesh = this->GetInput();
      OutputMeshPointer outputMesh = this->GetOutput();

      if (!inputMesh) {
        itkExceptionMacro(<< "Missing Input Mesh");
      }

      if (!outputMesh) {
        itkExceptionMacro(<< "Missing Output Mesh");
      }

      outputMesh->SetBufferedRegion(outputMesh->GetRequestedRegion());

      itk::Statistics::NormalVariateGenerator::Pointer randn = itk::Statistics::NormalVariateGenerator::New();
      randn->Initialize(m_RandomSeed);

      this->InitilaizeTransform();

      InputPointsContainerConstPointer inpPoints = inputMesh->GetPoints();
      OutputPointsContainerPointer     outPoints = outputMesh->GetPoints();
      outPoints->Reserve(inputMesh->GetNumberOfPoints());
      outPoints->Squeeze();

      typename InputPointsContainer::ConstIterator inputPoint = inpPoints->Begin();
      typename OutputPointsContainer::Iterator outputPoint = outPoints->Begin();

      for ( ; inputPoint != inpPoints->End(); ++inputPoint, ++outputPoint ) 
      {
        typename OutputMeshType::PointType point = m_Transform->TransformPoint(inputPoint.Value());

        if (m_StandardDeviation > itk::NumericTraits<double>::epsilon())
        {
          for (size_t n = 0; n < Dimension; ++n) 
          {
            point[n] += m_StandardDeviation * randn->GetVariate();
          }
        }

        outputPoint.Value() = point;
      }

      // Create duplicate references to the rest of data on the mesh
      this->CopyInputMeshToOutputMeshPointData();
      this->CopyInputMeshToOutputMeshCellLinks();
      this->CopyInputMeshToOutputMeshCells();
      this->CopyInputMeshToOutputMeshCellData();

      size_t maxDimension = TInputMesh::MaxTopologicalDimension;

      for (size_t n = 0; n < maxDimension; n++) {
        outputMesh->SetBoundaryAssignments(n, inputMesh->GetBoundaryAssignments(n));
      }
    }

    void PrintSelf(std::ostream & os, itk::Indent indent) const ITK_OVERRIDE
    {
      Superclass::PrintSelf(os, indent);
      if (m_Transform) {
        os << indent << "Transform: " << m_Transform << std::endl;
      }
    }

    void InitilaizeTransform()
    {
      if (!m_Transform) {
        itkExceptionMacro(<< "Missing Input Transform");
      }

      if (!m_TransformGenerator) {
        m_TransformGenerator = TransformGeneratorType::New();
        m_TransformGenerator->Initialize(m_RandomSeed);
      }

      typename TransformType::ParametersType parameters = m_Transform->GetParameters();

      for (size_t n = 0; n < m_Transform->GetNumberOfParameters(); ++n) {
        parameters[n] = m_TransformGenerator->GetUniformVariate(m_LowerBounds[n], m_UpperBounds[n]);
      }

      m_Transform->SetParameters(parameters);
    }

    /** Transform to apply to all the mesh points. */
    typename TransformType::Pointer m_Transform;
    TransformGeneratorType::Pointer m_TransformGenerator;
    ParametersType m_LowerBounds;
    ParametersType m_UpperBounds;

    double m_StandardDeviation = 1;
    int m_RandomSeed = 0;

  private:
    RandomTransformMeshFilter(const RandomTransformMeshFilter &) ITK_DELETE_FUNCTION;
    void operator=(const RandomTransformMeshFilter &)ITK_DELETE_FUNCTION;
  };
}