terraform {
  required_version = "~> 0.11.0"

  backend "s3" {
    dynamodb_table = "TerraformLock"
  }
}
