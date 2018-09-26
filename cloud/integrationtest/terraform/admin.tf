resource "aws_security_group" "admin" {
  name        = "loadtest_admin"
  vpc_id      = "${aws_vpc.main.id}"

  // Allow SSH
  ingress {
    from_port   = 22
    to_port     = 22
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

data "aws_ami" "ubuntu" {
  most_recent = true

  filter {
    name   = "name"
    values = ["ubuntu/images/hvm-ssd/ubuntu-bionic-18.04-amd64-server-*"]
  }

  filter {
    name   = "virtualization-type"
    values = ["hvm"]
  }

  owners = ["099720109477"]
}

resource "aws_instance" "admin" {
  ami           = "${data.aws_ami.ubuntu.id}"
  instance_type = "t2.micro"

  vpc_security_group_ids = ["${aws_security_group.admin.id}"]
  associate_public_ip_address = true
  key_name = "load_test"
  subnet_id = "${aws_subnet.public.0.id}" // TODO: is this notation ok?

  tags {
    Name = "load_test_admin"
  }
}
