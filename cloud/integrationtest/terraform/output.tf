output "admin_address" {
  description = "Public DNS for the admin host"
  value       = "${aws_instance.admin.public_dns}"
}
