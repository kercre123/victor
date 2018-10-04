data "aws_availability_zones" "available" {}

resource "aws_vpc" "main" {
  cidr_block = "172.16.0.0/16"

  enable_dns_support = "true"
  enable_dns_hostnames = "true"

  tags {
    Name = "load_test"
  }
}
// Public subnets host the NAT gateways
resource "aws_subnet" "public" {
  cidr_block        = "${cidrsubnet(aws_vpc.main.cidr_block, 4, count.index)}"
  vpc_id            = "${aws_vpc.main.id}"
  count             = "${var.az_count}"
  availability_zone = "${data.aws_availability_zones.available.names[count.index]}"

  tags {
    Name = "load_test_public_${count.index}"
  }
}

// Private subnets host the docker containers (Fargate cluster)
resource "aws_subnet" "private" {
  cidr_block        = "${cidrsubnet(aws_vpc.main.cidr_block, 4, var.az_count + count.index)}"
  vpc_id            = "${aws_vpc.main.id}"
  count             = "${var.az_count}"
  availability_zone = "${data.aws_availability_zones.available.names[count.index]}"

  tags {
    Name = "load_test_private_${count.index}"
  }
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

  tags {
    Name = "load_test"
  }
}
