
#include "coretech/vision/neuralnets/neuralNetModel_interface.h"
#include "util/logging/logging.h"
#include <fstream>

namespace Anki {
namespace Vision {
  
#define LOG_CHANNEL "NeuralNets"
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result INeuralNetModel::ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out)
{
  std::ifstream file(fileName);
  if (!file)
  {
    PRINT_NAMED_ERROR("NeuralNetModel.ReadLabelsFile.LabelsFileNotFound", "%s", fileName.c_str());
    return RESULT_FAIL;
  }
  
  labels_out.clear();
  std::string line;
  while (std::getline(file, line)) {
    labels_out.push_back(line);
  }
  
  LOG_INFO("NeuralNetModel.ReadLabelsFile.Success", "Read %d labels", (int)labels_out.size());
  
  return RESULT_OK;
}

} // namespace Vision
} // namespace Anki
