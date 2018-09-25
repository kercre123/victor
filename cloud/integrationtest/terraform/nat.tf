resource "aws_eip" "nat" {
  vpc = true
}

resource "aws_internet_gateway" "gw" {
  vpc_id = "${aws_vpc.main.id}"
}

resource "aws_nat_gateway" "nat" {
  subnet_id = "${aws_subnet.public.id}"
  allocation_id = "${aws_eip.nat.id}"

  depends_on = ["aws_internet_gateway.gw"]
}

resource "aws_route_table" "nat" {
  vpc_id = "${aws_vpc.main.id}"

  route {
    cidr_block = "0.0.0.0/0"
    nat_gateway_id = "${aws_nat_gateway.nat.id}"
  }
}

resource "aws_route_table_association" "nat" {
  subnet_id = "${aws_subnet.private.id}"
  route_table_id = "${aws_route_table.nat.id}"
}

resource "aws_route_table" "public" {
  vpc_id = "${aws_vpc.main.id}"

  route {
    cidr_block = "0.0.0.0/0"
    gateway_id = "${aws_internet_gateway.gw.id}"
  }
}

resource "aws_route_table_association" "public" {
    subnet_id = "${aws_subnet.public.id}"
    route_table_id = "${aws_route_table.public.id}"
}
