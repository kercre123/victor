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
