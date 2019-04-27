/**
 * Base class for objects which are event sources.
 */
class Observable {
  on(event, handler) {
    bean.on(this, event, handler)
  }

  fire(event, ...data) {
    bean.fire(this, event, ...data)
  }
}

/**
 * MetricSet encapsulates the set of selected metrics to be graphed
 * and the collected data for that graph.
 */
class MetricSet extends Observable {
  /** parent is a Metrics instance which this MetricSet is a subset of */
  constructor(parent) {
    super()
    this.parent = parent
    this.metrics = new Set()
    this.data = {}

    this.parent.on('update', () => {
      // Capture datapoints for any selected metrics
      for (let name of this.metrics) {
        this.store_point(name)
      }

      this.fire('update', this)
    })

    this.on('update', () => { this.parent.render() } )
  }

  add(metric) {
    if (!this.has(metric)) {
      this.metrics.add(metric)
      this.store_point(metric)
      this.fire('add', metric)
      this.fire('update')
    }
  }

  remove(metric) {
    if (this.has(metric)) {
      this.metrics.delete(metric)
      delete this.data[metric]
      this.fire('remove', metric)
      this.fire('update')
    }
  }

  toggle(metric) {
    if (this.has(metric)) {
      this.remove(metric)
    } else {
      this.add(metric)
    }
  }

  clear() {
    this.metrics.clear()
    this.data = {}
    this.fire('clear')
    this.fire('update')
  }

  clear_data() {
    for (let metric of this.metrics) {
      this.data[metric] = []
    }
    this.fire('update')
  }

  has(metric) {
    return this.metrics.has(metric)
  }

  store_point(metric) {
    let points = this.data[metric] || []
    points.push({
      timestamp: this.parent.timestamp,
      value: this.parent.metrics[metric]
    })
    this.data[metric] = points
  }

  plotly_data() {
    return Object.keys(this.data).map(name => {
      let raw = this.data[name]
      return {
        x: raw.map(point => point.timestamp),
        y: raw.map(point => point.value),
        type: 'scatter',
        name: name
      }
    })
  }
}

function process_metrics_response(rsp) {
  let metrics  = munge_metrics(rsp.data.metrics)
  let memstats = munge_memstats(rsp.data.memstats)
  let runtime  = munge_runtime(rsp.data.runtime)
  Object.assign(metrics, memstats, runtime)
  return metrics
}

/** Process the metrics into a flat key/value object, converting
 * metrics that are possibly in structured form. */
function munge_metrics(metrics) {
  let result = {}
  for (let name of Object.keys(metrics)) {
    let value = metrics[name]
    if (typeof(value) == 'number') {
      result[name] = value
    } else if (typeof(value) == 'object') {
      for (let key of Object.keys(value)) {
        result[`${name}.${key}`] = value[key]
      }
    }
  }
  return result
}

/** Process the go memory stats into a flat key/value object,
 * prefixing them with go.memstats. */
function munge_memstats(memstats) {
  let result = {}
  for (let stat of Object.keys(memstats || {})) {
    let value = memstats[stat]
    if (typeof(value) == 'number') {
      let name = 'go.memstats.' + stat
      result[name] = value
    }
  }
  return result
}

/** Process the go runtime stats into a flat key/value object,
 * prefixing them with go.memstats. */
function munge_runtime(runtime) {
  let result = {}
  for (let stat of Object.keys(runtime || {})) {
    let value = runtime[stat]
    if (typeof(value) == 'number') {
      let name = 'go.runtime.' + stat
      result[name] = value
    }
  }
  return result
}

/**
 * Metrics encapsulates the set of all metrics and their most recent
 * values, fetched from the /debug/metrics endpoint.
 */
class Metrics extends Observable {
  constructor() {
    super()
    this.metrics = {}
    this.names = []
    this.timestamp = null
    this.period = 5000
    this.filter = null
  }

  value(metric) {
    return this.metrics[metric]
  }

  update() {
    return axios.get('/debug/metrics')
      .then(rsp => {
        Data.options.plotly_layout.title = rsp.data.cmdline.join(' ')
        return rsp
      })
      .then(process_metrics_response)
      .then(metrics => {
        this.metrics = metrics
        this.names = Object.keys(metrics)
        this.timestamp = new Date()
            this.render()
        this.fire('update', this)
      })
  }

  start() {
    stop()
    this.interval = window.setInterval(() => this.update(), this.period)
  }

  stop() {
    if (this.interval) {
      window.clearInterval(this.interval)
      this.interval = null
    }
  }

  set_period(period) {
    this.period = period
    if (this.is_running()) {
      this.stop()
      this.start()
    }
  }

  is_running() {
    return Boolean(this.interval)
  }

  setFilter(filter) {
    this.filter = filter
    this.render()
    set_url()
  }

  render() {
    let re = new RegExp(this.filter || '.*')
    let metrics = this.names.map(name => {
        return {
          name: name,
          value: this.value(name),
          selected: Data.selected.has(name)
        }
      }).filter(metric => re.exec(metric.name))

    $('#metrics-count').text(this.names.length)
    $('#filtered-count').text(metrics.length)

    $('#metric-table').html(templates.metric_table_template({
      data: metrics
    }))

    let input = $('#input-filter')
    if (input.val() != this.filter) {
      input.val(this.filter)
    }
  }
}

/* ----------------------------------------------------------------------
   Plotting
   ---------------------------------------------------------------------- */

function _plot(data) {
  return Plotly.newPlot('chart', data, Data.options.plotly_layout, Data.options.plotly)
}

function plot_line_chart() {
  return _plot(Data.selected.plotly_data())
}

function plot_area_chart() {
  let stackedArea = function(traces) {
    if (traces && traces.length) {
      traces[0].fill = 'tozeroy'
      traces.forEach(series => {series.y.reverse(); series.x.reverse() })
	  for(var i=1; i<traces.length; i++) {
        traces[i].fill = 'tonexty'
	    for(var j=0; j<(Math.min(traces[i]['y'].length, traces[i-1]['y'].length)); j++) {
		  traces[i]['y'][j] += traces[i-1]['y'][j];
	    }
	  }
      traces.forEach(series => {series.y.reverse(); series.x.reverse() })
    }
	return traces;
  }
  return _plot(stackedArea(Data.selected.plotly_data()))
}

function plot_bar_chart() {
  let data = Data.selected.plotly_data()
  for (let series of data) {
    series.type = 'bar'
  }
  return _plot(data)
}

var plot_fn = plot_line_chart

function plot() {
  return plot_fn()
}

/* ----------------------------------------------------------------------
   URL Handling
   ---------------------------------------------------------------------- */

function set_url() {
  let u = new URI()
  u.removeQuery('metric')
  u.removeQuery('filter')
  if (Data.selected.metrics.size > 0) {
    u.addQuery('metric', Array.from(Data.selected.metrics.values()))
  }
  if (Data.metrics.filter) {
    u.addQuery('filter', Data.metrics.filter)
  }
  history.pushState({}, '', u.toString())
}

function parse_url() {
  let q = new URI().query(true)
  if (q.metric) {
    if (typeof(q.metric) == 'string') {
      Data.selected.add(q.metric)
    } else {
      for (let metric of q.metric) {
        Data.selected.add(metric)
      }
    }
  }
  if (q.filter) {
    Data.metrics.setFilter(q.filter)
  }
}

/* ----------------------------------------------------------------------
   Setup & Event Handlers
   ---------------------------------------------------------------------- */

function init() {
  templates = {
    metric_table_template: Handlebars.compile($('#metric-table-template').html())
  }
  Data = {
    options: {
      plotly: {
        showLink: false,
        modeBarButtonsToRemove: ['toImage', 'sendDataToCloud'],
        displaylogo: false
      },
      plotly_layout: {
      }
    },
    metrics: new Metrics()
  }
  Data.selected = new MetricSet(Data.metrics)
  Data.selected.on('update', plot)
  Data.selected.on('update', set_url)
  Data.metrics.start()
  Data.metrics.update()
  parse_url()
}

$(document).ready(() => {
  init()

  // Toggle metric selection when clicking on them in the metrics
  // table
  $(document).on('click', 'tr.metric-row td.name', (e) => {
    Data.selected.toggle(e.target.textContent)
  })

  // Set the metrics list filter
  $(document).on('input', '#input-filter', (e) => {
    Data.metrics.setFilter(e.currentTarget.value)
  })

  // Set the refresh period
  $(document).on('input', '#input-period', (e) => {
    Data.metrics.set_period(e.currentTarget.value * 1000)
  })

  // Clear just the chart data, leaving the metric selection intact
  $(document).on('click', '#btn-clear-data', (e) => {
    Data.selected.clear_data()
  })

  // Clear the chart and metric selection
  $(document).on('click', '#btn-clear', (e) => {
    Data.selected.clear()
  })

  // Toggle data collection
  $(document).on('click', '#btn-pause', (e) => {
    let icon = $('#btn-pause i')
    if (Data.metrics.is_running()) {
      Data.metrics.stop()
      icon.removeClass('fa-pause')
      icon.addClass('fa-play')
    } else {
      Data.metrics.start()
      icon.removeClass('fa-play')
      icon.addClass('fa-pause')
    }
  })

  // Clear Filter
  $(document).on('click', '#btn-clear-filter', (e) => {
    $('#input-filter').val('')
    Data.metrics.setFilter(null)
  })

  // Export a static image
  $(document).on('click', '#btn-export-png', (e) => {
    plot()
      .then(gd =>
            Plotly.toImage(gd, {
              format:'png',
              height: 600,
              width: 1400
            }))
      .then((data_url) => {
        let data = window.atob(data_url.split(',')[1])
        let buf  = new ArrayBuffer(data.length)
        let view = new Uint8Array(buf)
        for ( let i = 0; i < data.length; i++ ) {
          view[i] = data.charCodeAt(i) & 0xff
        }
        let blob = new Blob([view], {type:'image/png'})
        window.saveAs(blob, 'go-metrics.png')
      })
  })

  // Switch to line chart
  $(document).on('click', '#btn-line-chart', (e) => {
    $('.chart-button').removeClass('is-active')
    $('#btn-line-chart').addClass('is-active')
    plot_fn = plot_line_chart
    plot()
  })

  // Switch to area chart
  $(document).on('click', '#btn-area-chart', (e) => {
    $('.chart-button').removeClass('is-active')
    $('#btn-area-chart').addClass('is-active')
    plot_fn = plot_area_chart
    plot()
  })

  // Switch to bar chart
  $(document).on('click', '#btn-bar-chart', (e) => {
    $('.chart-button').removeClass('is-active')
    $('#btn-bar-chart').addClass('is-active')
    plot_fn = plot_bar_chart
    plot()
  })

  // Redraw the plot on window resize
  $(window).resize(plot)
})
