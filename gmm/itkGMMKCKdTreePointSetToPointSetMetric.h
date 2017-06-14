#ifndef itkGMMKCKdTreePointSetToPointSetMetric_h
#define itkGMMKCKdTreePointSetToPointSetMetric_h

#include <itkPointsLocator.h>

#include "itkGMMPointSetToPointSetMetricBase.h"

namespace itk
{
/** \class GMMRigidPointSetToPointSetMetric
 * \brief Computes similarity between pixel values of a point set and
 * intensity values of an image.
 *
 * This metric computes the average squared differences between pixels
 * in the point set and the transformed set of pixels.
 *
 * Spatial correspondence between both images is established through a
 * Transform.
 */
template< typename TFixedPointSet, typename TMovingPointSet >
class GMMKCKdTreePointSetToPointSetMetric : public GMMPointSetToPointSetMetricBase < TFixedPointSet, TMovingPointSet >
{
public:
  /** Standard class typedefs. */
  typedef GMMKCKdTreePointSetToPointSetMetric                                Self;
  typedef GMMPointSetToPointSetMetricBase< TFixedPointSet, TMovingPointSet > Superclass;
  typedef SmartPointer< Self >                                               Pointer;
  typedef SmartPointer< const Self >                                         ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(GMMKCKdTreePointSetToPointSetMetric, GMMPointSetToPointSetMetricBase);

  /** Types transferred from the base class */
  typedef typename Superclass::TransformType              TransformType;
  typedef typename Superclass::TransformPointer           TransformPointer;
  typedef typename Superclass::TransformParametersType    TransformParametersType;
  typedef typename Superclass::TransformJacobianType      TransformJacobianType;
  typedef typename Superclass::InputPointType             InputPointType;
  typedef typename Superclass::OutputPointType            OutputPointType;
  typedef typename Superclass::MeasureType                MeasureType;
  typedef typename Superclass::DerivativeType             DerivativeType;
  typedef typename Superclass::FixedPointSetType          FixedPointSetType;
  typedef typename Superclass::MovingPointSetType         MovingPointSetType;
  typedef typename Superclass::FixedPointsContainer       FixedPointsContainer;
  typedef typename Superclass::MovingPointsContainer      MovingPointsContainer;
  typedef typename Superclass::FixedPointSetConstPointer  FixedPointSetConstPointer;
  typedef typename Superclass::MovingPointSetConstPointer MovingPointSetConstPointer;
  typedef typename Superclass::FixedPointIterator         FixedPointIterator;
  typedef typename Superclass::MovingPointIterator        MovingPointIterator;

  typedef typename Superclass::LocalDerivativeType        LocalDerivativeType;
  typedef typename Superclass::GradientType               GradientType;

  typedef itk::PointsLocator<typename FixedPointSetType::PointsContainer>  FixedPointsLocatorType;
  typedef itk::PointsLocator<typename MovingPointSetType::PointsContainer> MovingPointsLocatorType;

  /** Set/Get radius. */
  itkSetMacro(Radius, double);
  itkGetMacro(Radius, double);

  /** Get the derivatives of the match measure. */
  void GetDerivative(const TransformParametersType & parameters, DerivativeType & Derivative) const ITK_OVERRIDE;

  /**  Get the value for single valued optimizers. */
  MeasureType GetValue(const TransformParametersType & parameters) const ITK_OVERRIDE;

  /**  Get value and derivatives for multiple valued optimizers. */
  void GetValueAndDerivative(const TransformParametersType & parameters, MeasureType & Value, DerivativeType & Derivative) const ITK_OVERRIDE;

  /** Initialize the Metric by making sure that all the components
  *  are present and plugged together correctly     */
  virtual void Initialize(void)
  throw (ExceptionObject);

protected:
  GMMKCKdTreePointSetToPointSetMetric();
  virtual ~GMMKCKdTreePointSetToPointSetMetric() {}

  typename FixedPointsLocatorType::Pointer m_FixedPointsLocator;
  typename MovingPointsLocatorType::Pointer m_TransformedPointsLocator;
  typename MovingPointsContainer::Pointer m_TransformedPointsContainer;

  double m_Radius = 3;

private:
  GMMKCKdTreePointSetToPointSetMetric(const Self &) ITK_DELETE_FUNCTION;
  void operator=(const Self &) ITK_DELETE_FUNCTION;
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGMMKCKdTreePointSetToPointSetMetric.hxx"
#endif

#endif