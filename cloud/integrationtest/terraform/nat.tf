resource "aws_eip" "nat" {
  count = "${var.az_count}"
  vpc   = true

  depends_on = ["aws_internet_gateway.gw"]

  tags {
    Name = "load_test_${count.index}"
  }
}

resource "aws_internet_gateway" "gw" {
  vpc_id = "${aws_vpc.main.id}"

  tags {
    Name = "load_test"
  }
}

resource "aws_nat_gateway" "nat" {
  count         = "${var.az_count}"

  subnet_id     = "${element(aws_subnet.public.*.id, count.index)}"
  allocation_id = "${element(aws_eip.nat.*.id, count.index)}"

  depends_on = ["aws_internet_gateway.gw"]

  tags {
    Name = "load_test_${count.index}"
  }
}

resource "aws_route_table" "private" {
  count  = "${var.az_count}"

  vpc_id = "${aws_vpc.main.id}"

  route {
    cidr_block = "0.0.0.0/0"
    nat_gateway_id = "${element(aws_nat_gateway.nat.*.id, count.index)}"
  }

  tags {
    Name = "load_test_private_${count.index}"
  }
}

resource "aws_route_table_association" "nat" {
  count          = "${var.az_count}"

  subnet_id      = "${element(aws_subnet.private.*.id, count.index)}"
  route_table_id = "${element(aws_route_table.private.*.id, count.index)}"
}

resource "aws_route_table" "public" {
  count  = "${var.az_count}"

  vpc_id = "${aws_vpc.main.id}"

  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = "${aws_internet_gateway.gw.id}"
  }

  tags {
    Name = "load_test_public_${count.index}"
  }
}

resource "aws_route_table_association" "public" {
  count  = "${var.az_count}"

  subnet_id      = "${element(aws_subnet.public.*.id, count.index)}"
  route_table_id = "${element(aws_route_table.public.*.id, count.index)}"
}
