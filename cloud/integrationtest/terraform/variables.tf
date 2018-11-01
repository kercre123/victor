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
  default = 10
}

// Note: number of container instances per cluster: 1000
variable "instance_count" {
  description = "Number of load test Docker containers running per task"
  default = 2
}

// Note: number of tasks using the Fargate launch type, per region, per account: 20 (ECS=1000)
variable "service_count" {
  description = "Number of load test services running in cluster"
  default = 2
}

variable "app_image" {
  default = "649949066229.dkr.ecr.us-west-2.amazonaws.com/load_test:latest"
}

variable "logging_role" {
  default = "arn:aws:iam::792379844846:role/cross-account-kinesis-logging-development"
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
