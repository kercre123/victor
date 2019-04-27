# go-metrics-charts

This project provides It real-time live charting of internal metrics
for applications that use the
[rcrowley/go-metrics](https://github.com/rcrowley/go-metrics) metrics
library. It began as a fork of
[github.com/mkevac/debugcharts](http://github.com/mkevac/debugcharts),
until it seemed cleaner to keep it separate.

This is intended as a simple live view of metrics in a single process
without the need for an entire metrics stack - it can be useful when
developing and testing locally. All third party dependencies are
loaded from a CDN to keep the amount of embedded data to a minimum.

## Browser Compatibility

The javascript code is written in ES6, so it requires a
[modern browser](http://kangax.github.io/compat-table/es6/). It's been
tested in Safari 10 and Chrome 54.

## Installation

`go get github.com/aalpern/go-metrics-charts`

## Usage

```
import "github.com/aalpern/go-metrics-charts"

metricscharts.Register()
```

You must have metrics exposed via expvars, as rather than having a
binary depedency on the metrics package, the UI works by hitting the
`/debug/metrics` JSON endpoint exposed by that package.

```
import "github.com/rcrowley/go-metrics/exp"

exp.Exp(metrics.DefaultRegistry)
```

The live charts page will be registered at `/debug/metrics/charts`.

![screenshot](screenshot.png)

## TO-DO

* [ ] better handling of long list of metrics
* [x] set refresh period
* [ ] limit the data to a rolling window of <n> data points
* [x] Chart options - add stacked area, display data points
* [x] Responsive chart
* [x] pause/restart
* [x] merge runtime stats into metrics list for unified UI
* [x] implement filtering of metrics list
* [x] URL handling/generation
