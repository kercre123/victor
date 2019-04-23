variable "region" {
  default = "us-west-2"
}

// Note: Oregon (us-west-2) has three availability zones
variable "az_count" {
  description = "Number of AZs to use for deployment (maximize AZs per region)"
  default = 3
}

variable "robots_per_process" {
  description = "Number of robot instances per load test Docker container"
  default = 100
}

// Note: number of container instances per cluster: 1000
variable "instance_count" {
  description = "Number of load test Docker containers per Fargate cluster"
  default = 998
}

// Note: number of tasks using the Fargate launch type, per region, per account: 20 (ECS=1000)
variable "service_count" {
  description = "Number of load test services running per Fargate cluster"
  default = 1
}

variable "app_image" {
  default = "649949066229.dkr.ecr.us-west-2.amazonaws.com/load_test:latest"
}

variable "logging" {
  type    = "map"
  default = {
    role = "arn:aws:iam::792379844846:role/cross-account-kinesis-logging-loadtest"
    source = "robot_fleet"
    stream = "splunk_logs_loadtest"
    index = "sai_loadtest"
    type = "kinesis"
    source_type = "sai_go_general"
  }
}

// reporting_interval -> Metrics reporting interval for Wavefront (in Go's time.Duration string format)
// reporting_tasks -> Number of metrics reporting load test Docker containers per Fargate cluster (can be used to subsample metrics)
// Estimated steady state datapoint rate (i.e. excluding setup/teardown):
//    = (instance_count + (reporting_tasks * num_periodic_actions * datapoints_per_action)) / reporting_interval
//    = (998 + 3 * 5 * 15) / 30 = 40 datapoints per second
variable "wavefront" {
  type    = "map"
  default = {
    token = "DUMMY_WF_URL_API_TOKEN"
    url = "https://metrics.wavefront.com/api"
    reporting_tasks = 3
    reporting_interval = "30s"
  }
}

// Note: determines if a new account is created as part of the test action
variable "enable_account_creation" {
  default = "true"
}

// Note: determines if the containers are remote controlled via Redis
variable "enable_distributed_control" {
  default = "true"
}

variable "timer_params" {
  type    = "map"
  default = {
    "heart_beat_interval" = "0s"
    "heart_beat_stddev" = "0s"

    "jdocs_interval" = "5m"
    "jdocs_stddev" = "3m"

    "log_collector_interval" = "10m"
    "log_collector_stddev" = "4m"

    "token_refresh_interval" = "0s"
    "token_refresh_stddev" = "0s"

    "connection_check_interval" = "10m"
    "connection_check_stddev" = "5m"
  }
}

variable "ramp_durations" {
  description = "Duration for robots (across all containers) to ramp up/down (Go time.Duration format)"
  type    = "map"
  default = {
    "up" = "15m"
    "down" = "0s"
  }
}

// Fargate Pricing (us-west-2): per vCPU per hour $0.0506, per GB per hour $0.0127
// See (for supported configurations and pricing): https://aws.amazon.com/fargate/pricing/
// Total: service_count * instance_count * (fargate_cpu * $0.0506 + fargate_memory * $0.0127) per hour
// Example (1 service, 1000 containers): 1 * 1000 * (0.25*0.0506 + 0.5*0.0127) = $19 per hour
variable "fargate_cpu" {
  description = "Fargate instance CPU units to provision (1 vCPU = 1024 CPU units)"
  default     = "256"
}

variable "fargate_memory" {
  description = "Fargate instance memory to provision (in MiB)"
  default     = "512"
}
