resource "aws_ecs_cluster" "load_test" {
  name = "load_test"
}

///////////////////////////////////////////////////////////////////////////// Load Test Service

data "template_file" "load_test" {
  template = "${file("task-definitions/load-test.json")}"

  vars {
    region = "${var.region}"

    image = "${var.app_image}"
    cpu = "${var.fargate_cpu}"
    memory = "${var.fargate_memory}"

    enable_account_creation = "${var.enable_account_creation}"
  }
}

resource "aws_ecs_task_definition" "load_test" {
  family                   = "load_test"
  container_definitions    = "${data.template_file.load_test.rendered}"

  execution_role_arn       = "${aws_iam_role.ecs_execution.arn}"
  network_mode             = "awsvpc"
  requires_compatibilities = ["FARGATE"]
  cpu                      = "${var.fargate_cpu}"
  memory                   = "${var.fargate_memory}"
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
    subnets          = ["${aws_subnet.private.id}"]
  }
}

///////////////////////////////////////////////////////////////////////////// Redis Service

data "template_file" "redis" {
  template = "${file("task-definitions/redis.json")}"

  vars {
    cpu = "${var.fargate_cpu}"
    memory = "${var.fargate_memory}"
  }
}

resource "aws_ecs_task_definition" "redis" {
  family                   = "redis"
  container_definitions    = "${data.template_file.redis.rendered}"

  network_mode             = "awsvpc"
  requires_compatibilities = ["FARGATE"]
  cpu                      = 256
  memory                   = 512
}

resource "aws_ecs_service" "redis" {
  name            = "redis"
  cluster         = "${aws_ecs_cluster.load_test.id}"
  task_definition = "${aws_ecs_task_definition.redis.arn}"
  desired_count   = 1
  launch_type     = "FARGATE"

  network_configuration {
    security_groups  = ["${aws_security_group.ecs_tasks.id}"]
    subnets          = ["${aws_subnet.private.id}"]
  }

  service_registries {
    registry_arn    = "${aws_service_discovery_service.redis.arn}"
    container_name  = "redis"
  }
}

resource "aws_ecr_repository" "load_test" {
  name = "load_test"
}
