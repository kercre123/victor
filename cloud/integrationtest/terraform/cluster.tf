resource "aws_ecs_cluster" "load_test" {
  name = "load_test"
}

///////////////////////////////////////////////////////////////////////////// Load Test Service

data "template_file" "load_test" {
  template = "${file("task-definitions/load-test.json")}"

  vars {
    region = "${var.region}"

    image = "${var.app_image}"

    logging_role = "${var.logging["role"]}"
    logging_source = "${var.logging["source"]}"
    logging_stream = "${var.logging["stream"]}"
    logging_index = "${var.logging["index"]}"
    logging_type = "${var.logging["type"]}"
    logging_source_type = "${var.logging["source_type"]}"

    robots_per_process = "${var.robots_per_process}"
    tasks_per_cluster = "${var.instance_count * var.service_count}"

    reporting_tasks_per_cluster = "${var.wavefront["reporting_tasks"]}"
    metrics_reporting_interval = "${var.wavefront["reporting_interval"]}"

    ramp_up_duration = "${var.ramp_durations["up"]}"
    ramp_down_duration = "${var.ramp_durations["down"]}"

    heart_beat_interval = "${var.timer_params["heart_beat_interval"]}"
    heart_beat_stddev = "${var.timer_params["heart_beat_stddev"]}"
    jdocs_interval = "${var.timer_params["jdocs_interval"]}"
    jdocs_stddev = "${var.timer_params["jdocs_stddev"]}"
    log_collector_interval = "${var.timer_params["log_collector_interval"]}"
    log_collector_stddev = "${var.timer_params["log_collector_stddev"]}"
    token_refresh_interval = "${var.timer_params["token_refresh_interval"]}"
    token_refresh_stddev = "${var.timer_params["token_refresh_stddev"]}"
    connection_check_interval = "${var.timer_params["connection_check_interval"]}"
    connection_check_stddev = "${var.timer_params["connection_check_stddev"]}"

    enable_account_creation = "${var.enable_account_creation}"
    enable_distributed_control = "${var.enable_distributed_control}"
  }
}

resource "aws_ecs_task_definition" "load_test" {
  family                   = "load_test"
  container_definitions    = "${data.template_file.load_test.rendered}"

  task_role_arn            = "${aws_iam_role.ecs_task.arn}"
  execution_role_arn       = "${aws_iam_role.ecs_execution.arn}"
  network_mode             = "awsvpc"

  // Fargate required options
  requires_compatibilities = ["FARGATE"]
  cpu                      = "${var.fargate_cpu}"     // CPU Units
  memory                   = "${var.fargate_memory}"  // MiB
}

resource "aws_ecs_service" "load_test" {
  count = "${var.service_count}"

  name            = "load_test_${count.index}"
  cluster         = "${aws_ecs_cluster.load_test.id}"
  task_definition = "${aws_ecs_task_definition.load_test.arn}"
  desired_count   = "${var.instance_count}"
  launch_type     = "FARGATE"

  network_configuration {
    security_groups  = ["${aws_security_group.ecs_tasks.id}"]
    subnets          = ["${aws_subnet.private.*.id}"]
  }
}

///////////////////////////////////////////////////////////////////////////// Redis Service

resource "aws_ecs_task_definition" "redis" {
  family                   = "redis"
  container_definitions    = "${file("task-definitions/redis.json")}"

  network_mode             = "awsvpc"

  // Fargate required options
  requires_compatibilities = ["FARGATE"]
  cpu                      = 256  // CPU Units
  memory                   = 512  // MiB
}

resource "aws_ecs_service" "redis" {
  name            = "redis"
  cluster         = "${aws_ecs_cluster.load_test.id}"
  task_definition = "${aws_ecs_task_definition.redis.arn}"
  desired_count   = 1
  launch_type     = "FARGATE"

  network_configuration {
    security_groups  = ["${aws_security_group.ecs_tasks.id}"]
    subnets          = ["${aws_subnet.private.*.id}"]
  }

  service_registries {
    registry_arn    = "${aws_service_discovery_service.redis.arn}"
    container_name  = "redis"
  }
}

resource "aws_ecr_repository" "load_test" {
  name = "load_test"
}

///////////////////////////////////////////////////////////////////////////// WaveFront Proxy

data "template_file" "wavefront" {
  template = "${file("task-definitions/wavefront.json")}"

  vars {
    wavefront_url = "${var.wavefront["url"]}"
    wavefront_token = "${var.wavefront["token"]}"
  }
}

resource "aws_ecs_task_definition" "wavefront" {
  family                   = "wavefront"
  container_definitions    = "${data.template_file.wavefront.rendered}"

  network_mode             = "awsvpc"

  // Fargate required options
  requires_compatibilities = ["FARGATE"]
  cpu                      = 1024  // CPU Units
  memory                   = 2048  // MiB
}

resource "aws_ecs_service" "wavefront" {
  name            = "wavefront"
  cluster         = "${aws_ecs_cluster.load_test.id}"
  task_definition = "${aws_ecs_task_definition.wavefront.arn}"
  desired_count   = 1
  launch_type     = "FARGATE"

  network_configuration {
    security_groups  = ["${aws_security_group.ecs_tasks.id}"]
    subnets          = ["${aws_subnet.private.*.id}"]
  }

  service_registries {
    registry_arn    = "${aws_service_discovery_service.wavefront.arn}"
    container_name  = "wavefront"
  }
}
