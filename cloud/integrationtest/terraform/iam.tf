// This role specifies what other services the ECS load test task can access, in this case
// assuming the cross-account Kinesis logging for development.
data "template_file" "ecs_task_role" {
  template = "${file("policy/AmazonECSTaskRolePolicy.json")}"

  vars {
    logging_role = "${var.logging["role"]}"
  }
}

resource "aws_iam_role" "ecs_task" {
  name = "ecs_task_role"
  assume_role_policy = "${file("policy/AmazonECSTaskRole.json")}"
}

resource "aws_iam_policy" "ecs_task" {
  name = "ecs_task_policy"
  policy = "${data.template_file.ecs_task_role.rendered}"
}

resource "aws_iam_policy_attachment" "ecs_task" {
  name = "ecs_task_role"
  roles = ["${aws_iam_role.ecs_task.name}"]
  policy_arn = "${aws_iam_policy.ecs_task.arn}"
}

// This role is required by Fargate tasks to pull container images and publish container logs to
// Amazon CloudWatch.
resource "aws_iam_role" "ecs_execution" {
  name = "ecs_execution_role"
  assume_role_policy = "${file("policy/AmazonECSTaskExecutionRole.json")}"
}

resource "aws_iam_policy" "ecs_execution" {
  name = "ecs_execution_policy"
  policy = "${file("policy/AmazonECSTaskExecutionRolePolicy.json")}"
}

resource "aws_iam_policy_attachment" "ecs_execution" {
  name = "ecs_execution_role"
  roles = ["${aws_iam_role.ecs_execution.name}"]
  policy_arn = "${aws_iam_policy.ecs_execution.arn}"
}
