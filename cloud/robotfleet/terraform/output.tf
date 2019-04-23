output "admin_address" {
  value = "${aws_instance.admin.public_dns}"
}
