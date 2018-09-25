resource "aws_vpc" "main" {
  cidr_block = "10.0.0.0/16"
}

// Public subnet hosts the NAT gateway
resource "aws_subnet" "public" {
  vpc_id = "${aws_vpc.main.id}"
  cidr_block = "10.0.1.0/24"
}

// Private subnet hosts the docker containers
resource "aws_subnet" "private" {
  vpc_id = "${aws_vpc.main.id}"
  cidr_block = "10.0.2.0/24"
}

resource "aws_security_group" "ecs_tasks" {
  name        = "ecs-tasks"
  vpc_id      = "${aws_vpc.main.id}"

  egress {
    protocol    = "-1"
    from_port   = 0
    to_port     = 0
    cidr_blocks = ["0.0.0.0/0"]
  }
}
