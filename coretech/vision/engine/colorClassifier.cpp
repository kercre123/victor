/**
 * File: colorClassifier.cpp
 *
 * Author: Patrick Doran
 * Date: 11/22/2018
 */

#include "coretech/vision/engine/colorClassifier.h"
#include "coretech/vision/engine/image.h"
#include "util/fileUtils/fileUtils.h"

#include <unordered_map>
#include <iterator>
#include <fstream>

namespace Anki
{
namespace Vision
{

const ColorClassifier::Label ColorClassifier::UNKNOWN_LABEL = { "background", PixelRGB(0,0,0) };

namespace
{

float MahalanobisDistanceSquared(const cv::Vec3f& sample, const cv::Vec3f& mean, const cv::Matx33f inv_cov)
{
  cv::Vec3f diff = sample-mean;
  return (diff.t() * inv_cov * diff).val[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Struct for containing label location
 */
struct LabelRegion
{
  std::string label;
  Anki::Rectangle<s32> rectangle;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/**
 * @brief Struct for containing stats about the label
 */
struct LabelStats
{
  std::string label;
  std::vector<cv::Vec3f> samples;
  cv::Vec3f mean;
  cv::Matx33f cov;
  cv::Matx33f inv_cov;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Load labels from a Labelme json file
 */
bool LoadLabels(const std::string& filename, std::unordered_map<std::string,std::vector<LabelRegion>>& labels)
{
  Json::CharReaderBuilder rbuilder;
  rbuilder["collectComments"] = false;
  std::string errs;
  std::ifstream is(filename);
  Json::Value root;

  if (!Json::parseFromStream(rbuilder, is, &root, &errs))
  {
    return false;
  }

  Json::Value shapes = root.get("shapes",Json::Value::null);
  if (shapes.isNull() || !shapes.isArray())
  {
    return false;
  }

  for (auto ii = 0; ii < shapes.size(); ++ii)
  {
    LabelRegion newLabel;

    Json::Value shape = shapes[ii];

    Json::Value type = shape.get("shape_type",Json::Value::null);
    if (type.isNull() || !type.isString() || type.asString() != "rectangle")
    {
      continue;
    }

    Json::Value label = shape.get("label",Json::Value::null);
    if (label.isNull() || !label.isString())
    {
      continue;
    }
    newLabel.label = label.asString();

    Json::Value points = shape.get("points",Json::Value::null);
    if (points.isNull() || !points.isArray() || points.size() != 2)
    {
      continue;
    }
    Json::Value p0 = points[0];
    Json::Value p1 = points[1];
    if (!p0.isArray() || p0.size() != 2 || !p1.isArray() || p1.size() != 2)
    {
      continue;
    }

    float x0 = p0[0].asInt();
    float y0 = p0[1].asInt();
    float x1 = p1[0].asInt();
    float y1 = p1[1].asInt();

    newLabel.rectangle = Anki::Rectangle<s32>(x0, y0, x1-x0, y1-y0);

    auto iter = labels.find(newLabel.label);
    if (iter == labels.end())
    {
      labels.emplace(newLabel.label, std::vector<LabelRegion>());
    }
    labels[newLabel.label].push_back(newLabel);
  }

  return true;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**
 * @brief Get the samples from the image within the region
 */
void GetSamples(const ImageRGB& image, LabelRegion& region, LabelStats& stats)
{
  ImageRGB subimage = image.GetROI(region.rectangle);
  for (int row = 0; row < subimage.GetNumRows(); ++row)
  {
    for (int col = 0; col < subimage.GetNumCols(); ++col)
    {
      PixelRGB& pixel = subimage(row,col);
      cv::Vec3f rgb(pixel.r() / 255.f, pixel.g() / 255.f, pixel.b() / 255.f);
      stats.samples.push_back(rgb);
    }
  }
}

void FillOutStats(LabelStats& stats)
{
  cv::Mat covar, mean;
  int flags = CV_COVAR_NORMAL | CV_COVAR_ROWS;
  int ctype = CV_32FC1;
  // TODO figure out why the channels don't work
  cv::calcCovarMatrix(cv::Mat((int)stats.samples.size(),3,CV_32F,stats.samples.data()), covar,mean,flags, ctype);

  stats.mean = mean;
  stats.cov = covar;
  stats.inv_cov = stats.cov.inv();

  cv::Mat w,u,vt;
  cv::SVD::compute(covar, w, u, vt, cv::SVD::FULL_UV);
//  std::cerr<<"w: "<<std::endl<<w<<std::endl;
//  std::cerr<<"u: "<<std::endl<<u<<std::endl;
//  std::cerr<<"vt: "<<std::endl<<vt<<std::endl;
}

} /* anonymous namespace */

ColorClassifier::ColorClassifier ()
{
  Reset();
}

ColorClassifier::~ColorClassifier ()
{
}

void ColorClassifier::Classify(const ImageRGB& image, Image& labels) const
{
  DEV_ASSERT(_trained,"Cannot classify, not trained");

  for (int row = 0; row < labels.GetNumRows(); ++row)
  {
    for (int col = 0; col < labels.GetNumCols(); ++col)
    {
      labels.get_CvMat_()(row,col) = Classify(image.get_CvMat_()(row,col));
    }
  }
}

u8 ColorClassifier::Classify (PixelRGB color) const
{
  std::vector<LabelStats> statistics;
  std::vector<float> distances(statistics.size());
  float threshold = 2.f; // Mahalanobis distance threshold. How many standard deviations away.
  threshold *= threshold; // Square the threshold because we're doing this with the distance squared

  for (auto i = 0; i < statistics.size(); ++i)
  {
    LabelStats& stats = statistics[i];
    cv::Vec3f rgb(color.r()/255.f, color.g()/255.f, color.b()/255.f);
    distances[i] = MahalanobisDistanceSquared(rgb,stats.mean,stats.inv_cov);
  }

  // The index is of the min_element
  auto iter = std::min_element(distances.begin(), distances.end());
  if (iter == distances.end())
    return UNKNOWN_INDEX;
  u8 index = static_cast<u8>(std::distance(distances.begin(), iter));
  if (distances[index] > threshold)
    return UNKNOWN_INDEX;
  else
    return index;
}

void ColorClassifier::ColorImage (const Image& labels, ImageRGB& image) const
{
  DEV_ASSERT(_trained,"Cannot classify, not trained");

  for (int row = 0; row < labels.GetNumRows(); ++row)
  {
    for (int col = 0; col < labels.GetNumCols(); ++col)
    {
      std::vector<Label>::size_type index = labels.get_CvMat_()(row,col);
      image.get_CvMat_()(row,col) = _labels.at(index).color;
    }
  }
}

ColorClassifier::Label ColorClassifier::GetLabel(u8 index) const
{
  DEV_ASSERT(index == UNKNOWN_INDEX || (index >= 0 && index < _labels.size()), "Invalid index");

  if (index == UNKNOWN_INDEX)
    return UNKNOWN_LABEL;

  return _labels[index];
}

void ColorClassifier::Reset()
{
  _labels.clear();
  _trained = false;
}

void ColorClassifier::Train (const std::string& directory)
{
  Reset();

  // Load the image and annotation files from the directory
  std::vector<std::string> imageFilenames;
  std::vector<std::string> labelFilenames;
  {
    // TODO: Better file checking
    bool useFullPath = true;
    bool recursive = true;
    imageFilenames = Util::FileUtils::FilesInDirectory(directory, useFullPath, "jpg", recursive);
    labelFilenames = Anki::Util::FileUtils::FilesInDirectory(directory, useFullPath, "json", recursive);
    std::sort(imageFilenames.begin(), imageFilenames.end());
    std::sort(labelFilenames.begin(), labelFilenames.end());

    // TODO: Do a better check that the filenames match the images
    DEV_ASSERT((int)imageFilenames.size() == (int)labelFilenames.size(),
               "Not all images in directory have label files");
  }

  std::unordered_map<std::string, LabelStats> inputImageStats;
  ImageRGB inputImage;
  for (auto index = 0; index < imageFilenames.size(); ++index)
  {
    const std::string& imageFilename = imageFilenames[index];
    const std::string& labelFilename = labelFilenames[index];

    DEV_ASSERT(inputImage.Load(imageFilename) == Anki::Result::RESULT_OK, "Could not load file");

    std::unordered_map<std::string,std::vector<LabelRegion>> labels;

    bool loaded = LoadLabels(labelFilename, labels);
    if (!loaded || labels.empty())
    {
      // Skip the ones without labels
      continue;
    }

    // Get the samples for all the label regions
    for (auto& kv : labels)
    {
      const std::string& label = kv.first;
      std::vector<LabelRegion>& regions = kv.second;

      auto iter = inputImageStats.find(label);
      if (iter == inputImageStats.end())
      {
        LabelStats stats;
        stats.label = label;
        inputImageStats.emplace(label,stats);
      }

      for (auto& region : regions)
      {
        GetSamples(inputImage,region,inputImageStats[label]);
      }
    }
  }

  std::vector<std::string> labels;
  std::transform(inputImageStats.begin(), inputImageStats.end(), std::back_inserter(labels),
                 [](const auto& pair){ return pair.first; });

  for (const auto& label : labels)
  {
    // Input Image Stats
    {
      LabelStats& stats = inputImageStats.at(label);
      FillOutStats(stats);

    }
  }

}

void ColorClassifier::Save (const std::string& path) const
{

}

void ColorClassifier::Load (const std::string& path){
  Reset();

}

} /* namespace Vision */
} /* namespace Anki */
