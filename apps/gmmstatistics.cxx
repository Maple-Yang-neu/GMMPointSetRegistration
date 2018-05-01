﻿#include <itkEuler3DTransform.h>
#include <itkMesh.h>
#include <itkTransformMeshFilter.h>

#include "itkInitializeMetric.h"
#include "itkPointSetToPointSetMetrics.h"
#include "itkRandomTransformMeshFilter.h"
#include "itkGMMPointSetToPointSetRegistrationMethod.h"

#include "itkIOutils.h"
#include "argsCustomParsers.h"

const unsigned int Dimension = 3;
typedef itk::Mesh<float, Dimension> MeshType;
typedef itk::PointSet<MeshType::PixelType, Dimension> PointSetType;

int main(int argc, char** argv) {

  // parse input arguments
  args::ArgumentParser parser("GMM-based point set to point set registration.", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

  args::Group required(parser, "Required arguments:", args::Group::Validators::All);

  args::ValueFlag<std::string> argFixedFileName(required, "fixed", "The fixed mesh (point-set) file name", {'f', "fixed"});
  args::ValueFlag<std::string> argMovingFileName(required, "moving", "The moving mesh (point-set) file name", {'m', "moving"});
  args::ValueFlag<std::string> argOutputFileName(parser, "output", "The output mesh (point-set) file name", {'o', "output"});
  args::ValueFlag<std::string> argTargetFileName(parser, "target", "The target mesh (point-set) file name", {'t', "target"});

  args::ValueFlag<size_t> argNumberOfLevels(parser, "levels", "The number of levels", {"levels"});
  args::ValueFlag<size_t> argNumberOfIterations(parser, "iterations", "The number of iterations", {"iterations"}, 1000);
  args::ValueFlag<size_t> argNumberOfEvaluations(parser, "evaluations", "The number of evaluations", {"evaluations"}, 1000);

  args::Flag trace(parser, "trace", "Optimizer iterations tracing", {"trace"});

  const std::string transformDescription =
    "The type of transform (That is number):\n"
    "  0 : Translation\n"
    "  1 : Versor3D\n"
    "  2 : Similarity\n"
    "  3 : ScaleSkewVersor3D\n";

  args::ValueFlag<size_t> argTypeOfTransform(parser, "transform", transformDescription, {"transform"}, 0);

  const std::string metricDescription =
    "The type of metric (That is number):\n"
    "  0 : L2Rigid\n"
    "  1 : L2\n"
    "  2 : KC\n";

  args::ValueFlag<size_t> argTypeOfMetric(parser, "metric", metricDescription, {"metric"}, 0);

  try {
    parser.ParseCLI(argc, argv);
  }
  catch (args::Help) {
    std::cout << parser;
    return EXIT_SUCCESS;
  }
  catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return EXIT_FAILURE;
  }
  catch (args::ValidationError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return EXIT_FAILURE;
  }

  std::string fixedFileName = args::get(argFixedFileName);
  std::string movingFileName = args::get(argMovingFileName);
  std::string targetFileName = args::get(argTargetFileName);
  size_t numberOfEvaluations = args::get(argNumberOfEvaluations);
  size_t numberOfIterations = args::get(argNumberOfIterations);
  size_t numberOfLevels = args::get(argNumberOfLevels);
  size_t typeOfTransform = args::get(argTypeOfTransform);
  size_t typeOfMetric = args::get(argTypeOfMetric);

  std::cout << "options" << std::endl;
  std::cout << "number of iterations " << numberOfIterations << std::endl;
  std::cout << "number of levels     " << numberOfLevels << std::endl;
  std::cout << "transform " << typeOfTransform << std::endl;
  std::cout << "metric    " << typeOfMetric << std::endl;
  std::cout << std::endl;

  //--------------------------------------------------------------------
  // read meshes
  MeshType::Pointer fixedMesh = MeshType::New();
  if (!readMesh<MeshType>(fixedMesh, fixedFileName)) {
    return EXIT_FAILURE;
  }

  std::cout << "fixed mesh " << fixedFileName << std::endl;
  std::cout << "number of points " << fixedMesh->GetNumberOfPoints() << std::endl;
  std::cout << std::endl;

  MeshType::Pointer movingMesh = MeshType::New();
  if (!readMesh<MeshType>(movingMesh, movingFileName)) {
    return EXIT_FAILURE;
  }

  std::cout << "moving mesh " << movingFileName << std::endl;
  std::cout << "number of points " << movingMesh->GetNumberOfPoints() << std::endl;
  std::cout << std::endl;

  //--------------------------------------------------------------------
  // initialize transform mesh filter
  typedef itk::Euler3DTransform <double> TransformType;
  TransformType::Pointer transform = TransformType::New();
  transform->SetIdentity();
  transform->SetCenter(fixedMesh->GetBoundingBox()->GetCenter());

  TransformType::ParametersType upperBounds(6);
  TransformType::ParametersType lowerBounds(6);

  for (int n = 0; n < 3; ++n) {
    lowerBounds[n] =-itk::Math::pi_over_4;
    upperBounds[n] = itk::Math::pi_over_4;
  }

  for (int n = 3; n < 6; ++n) {
    lowerBounds[n] = -100;
    upperBounds[n] = 100;
  }

  std::cout << "lower bounds " << lowerBounds << std::endl;
  std::cout << "upper bounds " << upperBounds << std::endl;

  typedef agtk::RandomTransformMeshFilter<MeshType, MeshType, TransformType> RandomTransformFilterType;
  RandomTransformFilterType::Pointer randomTransformMesh = RandomTransformFilterType::New();
  randomTransformMesh->SetInput(fixedMesh);
  randomTransformMesh->SetTransform(transform);
  randomTransformMesh->SetLowerBounds(lowerBounds);
  randomTransformMesh->SetUpperBounds(upperBounds);
  randomTransformMesh->SetStandardDeviation(1);

  //--------------------------------------------------------------------
  // initialize metric
  typedef itk::InitializeMetric<PointSetType, PointSetType> InitializeMetricType;
  InitializeMetricType::Pointer initializerMetric = InitializeMetricType::New();
  initializerMetric->SetTypeOfMetric(0);
  try {
    initializerMetric->Initialize();
  }
  catch (itk::ExceptionObject& excep) {
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
  }
  initializerMetric->Print(std::cout);
  InitializeMetricType::MetricType::Pointer metric = initializerMetric->GetMetric();

  //--------------------------------------------------------------------
  // compute statistics
  for (size_t count = 0; count < numberOfEvaluations; ++count) {

    // perform registration
    typedef itk::GMMPointSetToPointSetRegistrationMethod<PointSetType> GMMPointSetToPointSetRegistrationMethodType;
    GMMPointSetToPointSetRegistrationMethodType::Pointer registration = GMMPointSetToPointSetRegistrationMethodType::New();
    registration->SetFixedPointSet(fixedMesh->GetPoints());
    registration->SetMovingPointSet(movingMesh->GetPoints());
    registration->SetMetric(initializerMetric->GetMetric());
    registration->SetTransform(transform);
    registration->SetNumberOfLevels(numberOfLevels);
    try {
      registration->Update();
    }
    catch (itk::ExceptionObject& excep) {
      std::cerr << excep << std::endl;
      return EXIT_FAILURE;
    }

    // transform moving mesh
    typedef itk::TransformMeshFilter<MeshType, MeshType, TransformType> TransformMeshFilterType;
    TransformMeshFilterType::Pointer transformMesh = TransformMeshFilterType::New();
    transformMesh->SetInput(movingMesh);
    transformMesh->SetTransform(transform);
    try {
      transformMesh->Update();
    }
    catch (itk::ExceptionObject& excep) {
      std::cerr << excep << std::endl;
      return EXIT_FAILURE;
    }

    // compute metrics
    typedef itk::PointSetToPointSetMetrics<PointSetType> PointSetToPointSetMetricsType;
    PointSetToPointSetMetricsType::Pointer metrics = PointSetToPointSetMetricsType::New();
    metrics->SetFixedPointSet(fixedMesh->GetPoints());
    metrics->SetTargetPointSet(movingMesh->GetPoints());
    metrics->SetMovingPointSet(randomTransformMesh->GetOutput());
    metrics->Compute();
    metrics->PrintReport(std::cout);

    metrics->SetFixedPointSet(fixedMesh->GetPoints());
    metrics->SetTargetPointSet(movingMesh->GetPoints());
    metrics->SetMovingPointSet(transformMesh->GetOutput());
    metrics->Compute();
    metrics->PrintReport(std::cout);
  }

  return EXIT_SUCCESS;
}