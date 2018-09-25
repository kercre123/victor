resource "aws_ecs_cluster" "load_test" {
  name = "load_test"
}

data "template_file" "load_test" {
  template = "${file("task-definitions/service.json")}"

  vars {
    region = "${var.region}"
    test_id = "${count.index}"
    enable_account_creation = "${var.enable_account_creation}"
    image = "${var.app_image}"
    cpu = "${var.fargate_cpu}"
    memory = "${var.fargate_memory}"
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

resource "aws_ecr_repository" "load_test" {
  name = "load_test"
}
