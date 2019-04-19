
# SAI Robot Fleet
Virtual robot fleet for testing SAI services at scale. More detailed information can be found [here](https://ankiinc.atlassian.net/wiki/spaces/SAI/pages/464781603/Robot+Load+Testing+in+the+Cloud).

## Deployment
Changes can be applied as follows:

```Shell
aws-okta exec anki-lab-2-tf-full -- terraform apply
```

<!-- BEGINNING OF PRE-COMMIT-TERRAFORM DOCS HOOK -->
## Inputs

| Name | Description | Type | Default | Required |
|------|-------------|:----:|:-----:|:-----:|
| az\_count | Number of AZs to use for deployment (maximize AZs per region) | string | `"3"` | no |
| enable\_account\_creation | Note: determines if a new account is created as part of the test action | string | `"true"` | no |
| enable\_distributed\_control | Note: determines if the containers are remote controlled via Redis | string | `"true"` | no |
| fargate\_cpu | Fargate instance CPU units to provision (1 vCPU = 1024 CPU units) | string | `"256"` | no |
| fargate\_memory | Fargate instance memory to provision (in MiB) | string | `"512"` | no |
| instance\_count | Number of load test Docker containers per Fargate cluster | string | `"0"` | no |
| logging | Splunk logging related settings | map | `<map>` | no |
| ramp\_durations | Duration for robots (across all containers) to ramp up/down (Go time.Duration format) | map | `<map>` | no |
| region | Deployment region | string | `"us-west-2"` | no |
| robots\_per\_process | Number of robot instances per load test Docker container | string | `"100"` | no |
| service\_count | Number of load test services running per Fargate cluster | string | `"1"` | no |
| timer\_params | Test action related statistical settings (intervals and standard deviations) | map | `<map>` | no |
| wavefront | Wavefront monitoring related settings | map | `<map>` | no |

## Outputs

| Name | Description |
|------|-------------|
| admin\_address | Public DNS for the admin host |

<!-- END OF PRE-COMMIT-TERRAFORM DOCS HOOK -->
