#pragma once

#include "arf/Types.h"
#include "arf/BulletinBoard.h"
#include "arf/TimeLimitedTemporalBuffer.h"
#include <vector>

namespace ARF {

template <typename DataT, typename TimeT> class TimeAwareViewer {
public:

  using BufferType = TimeLimitedTemporalBuffer<DataT, TimeT>;
  using ViewerType = TopicViewer<BufferType>;

  TimeAwareViewer(ViewerType v) 
  : viewer_( v ) {}

  bool IsInitialized() const { return viewer_.IsInitialized(); }

  using DataSinceLastView = std::vector<DataT>;
  DataSinceLastView GetDataSinceLastView() {
    DataSinceLastView ret;
    if (!IsInitialized())
      return ret;
    viewer_.InvokeView(
        [&ret, this](const TimeLimitedTemporalBuffer<DataT, TimeT> &buffer) {
          if (time_is_valid(last_view_time_)) {
            const auto data_since_last =
                buffer.GetValuesNewerThan(this->last_view_time_);
            for (const auto &data : data_since_last) {
              ret.push_back(data.second);
            }
            if( !data_since_last.empty() ) {
              this->last_view_time_ = data_since_last.crbegin()->first;
            }
          } else {
            const auto &full_data_buffer = buffer.GetBuffer();
            for (const auto &data : full_data_buffer) {
              ret.push_back(data.second);
            }
            if (!ret.empty()) {
              this->last_view_time_ = full_data_buffer.crbegin()->first;
            }
          }
        });
    return ret;
  }

private:
  TimeT last_view_time_ = TimeT(); // initializes to 0 (invalid)
  TopicViewer<TimeLimitedTemporalBuffer<DataT, TimeT>> viewer_;
};

} // namespace ARF