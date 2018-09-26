resource "aws_vpc" "main" {
  cidr_block = "10.0.0.0/16"

  enable_dns_support = "true"
  enable_dns_hostnames = "true"
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

  // Allow access to Redis
  ingress {
    from_port   = 6379
    to_port     = 6379
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }


  egress {
    protocol    = "-1"
    from_port   = 0
    to_port     = 0
    cidr_blocks = ["0.0.0.0/0"]
  }
}
