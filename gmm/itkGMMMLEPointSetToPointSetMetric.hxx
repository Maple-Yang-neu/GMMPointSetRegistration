#ifndef itkGMMMLEPointSetToPointSetMetric_hxx
#define itkGMMMLEPointSetToPointSetMetric_hxx

#include "itkGMMMLEPointSetToPointSetMetric.h"

namespace itk
{
/**
 * Constructor
 */
template <typename TFixedPointSet, typename TMovingPointSet>
GMMMLEPointSetToPointSetMetric<TFixedPointSet, TMovingPointSet>::GMMMLEPointSetToPointSetMetric()
{
}

/** Initialize the metric */
template< typename TFixedPointSet, typename TMovingPointSet >
void
GMMMLEPointSetToPointSetMetric< TFixedPointSet, TMovingPointSet >
::Initialize(void)
throw (ExceptionObject)
{
  Superclass::Initialize();
}

/**
 * Get the match Measure
 */
template <typename TFixedPointSet, typename TMovingPointSet>
typename GMMMLEPointSetToPointSetMetric<TFixedPointSet, TMovingPointSet>::MeasureType
GMMMLEPointSetToPointSetMetric<TFixedPointSet, TMovingPointSet>::GetValue(const TransformParametersType & parameters) const
{
  itkExceptionMacro(<< "not implemented");
}
/**
 * Get the Derivative Measure
 */
template <typename TFixedPointSet, typename TMovingPointSet>
void GMMMLEPointSetToPointSetMetric<TFixedPointSet, TMovingPointSet>::GetDerivative(const TransformParametersType & parameters, DerivativeType & derivative) const
{
  itkExceptionMacro(<< "not implemented");
}

/*
 * Get both the match Measure and theDerivative Measure
 */
template <typename TFixedPointSet, typename TMovingPointSet>
void GMMMLEPointSetToPointSetMetric<TFixedPointSet, TMovingPointSet>::GetValueAndDerivative(const TransformParametersType & parameters, MeasureType & value, DerivativeType  & derivative) const
{
  m_Transform->SetParameters(parameters);

  if (derivative.size() != m_NumberOfParameters) {
    derivative.set_size(m_NumberOfParameters);
  }

  value = NumericTraits<MeasureType>::ZeroValue();
  double scale = 2*m_MovingPointSetScale*m_MovingPointSetScale;

  itk::Array<double> vector;
  vector.set_size(m_FixedPointSet->GetNumberOfPoints());

  for (FixedPointIterator fixedIter = m_FixedPointSet->GetPoints()->Begin(); fixedIter != m_FixedPointSet->GetPoints()->End(); ++fixedIter) {
    const typename FixedPointSetType::PointType fixedPoint = fixedIter.Value();
    double sum = 1.0e-05;

    for (MovingPointIterator movingIter = m_MovingPointSet->GetPoints()->Begin(); movingIter != m_MovingPointSet->GetPoints()->End(); ++movingIter) {
      const typename MovingPointSetType::PointType transformedPoint = m_Transform->TransformPoint(movingIter.Value());
      const double dist = transformedPoint.SquaredEuclideanDistanceTo(fixedPoint);
      sum += exp(-dist/scale);
    }

    vector[fixedIter.Index()] = sum;
    value -= log(sum);
  }

  m_Gradient.fill(0);

  for (MovingPointIterator movingIter = m_MovingPointSet->GetPoints()->Begin(); movingIter != m_MovingPointSet->GetPoints()->End(); ++movingIter) {
    const typename MovingPointSetType::PointType transformedPoint = m_Transform->TransformPoint(movingIter.Value());
    
    for (FixedPointIterator fixedIter = m_FixedPointSet->GetPoints()->Begin(); fixedIter != m_FixedPointSet->GetPoints()->End(); ++fixedIter) {
      const typename FixedPointSetType::PointType fixedPoint = fixedIter.Value();
      const double dist = transformedPoint.SquaredEuclideanDistanceTo(fixedPoint);
      const size_t row = fixedIter.Index();

      for (size_t dim = 0; dim < Self::MovingPointSetDimension; ++dim) {
        m_Gradient(row, dim) -= 2.0 * exp(-dist/scale) * (fixedPoint[dim] - transformedPoint[dim]) / scale / vector[row];
      }
    }
  }

  // compute the derivatives
  derivative.Fill(NumericTraits<typename DerivativeType::ValueType>::ZeroValue());

  for (MovingPointIterator movingIter = m_MovingPointSet->GetPoints()->Begin(); movingIter != m_MovingPointSet->GetPoints()->End(); ++movingIter) {
    m_Transform->ComputeJacobianWithRespectToParametersCachedTemporaries(movingIter.Value(), m_Jacobian, m_JacobianCache);
    const size_t row = movingIter.Index();

    for (size_t par = 0; par < m_NumberOfParameters; par++) {
      double &sum = derivative[par];

      for (size_t dim = 0; dim < Self::MovingPointSetDimension; dim++) {
        sum += m_Jacobian(dim, par) * m_Gradient(row, dim);
      }
    }
  }
}
}

#endif