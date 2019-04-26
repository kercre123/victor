package config

var defaultConfig = `
[common]
default_env = dev

[env.prod]
accounts_url = https://accounts.api.anki.com
ankival_url = https://ankival.api.anki.com
audit_url = https://audit.api.anki.com
virtualrewards_url = https://virtualrewards.api.anki.com
virtualrewards_queue_url =	https://sqs.us-west-2.amazonaws.com/792379844846/virtualrewards-production

user_key = po3ooTh4eupax1Zaeyee2u


[env.beta]
accounts_url = https://accounts-beta.api.anki.com
ankival_url = https://ankival-beta.api.anki.com
audit_url = https://audit-beta.api.anki.com
virtualrewards_url = https://virtualrewards-beta.api.anki.com

user_key = azaic5aetheikeePaiFaek

[env.dev]
accounts_url = https://accounts-dev2.api.anki.com
ankival_url = https://ankival-dev2.api.anki.com
audit_url = https://audit-dev.api.anki.com
blobstore_url = https://blobstore-dev.api.anki.com
virtualrewards_url = https://virtualrewards-dev.api.anki.com
virtualrewards_queue_url = https://sqs.us-west-2.amazonaws.com/792379844846/virtualrewards-development
jdocs_url = https://jdocs-dev.api.anki.com

user_key = aedie7miecieth4EiGooKo

[env.loadtest]
das_queue_url = https://sqs.us-west-2.amazonaws.com/792379844846/DasLoadTest-dasloadtestSqs
redshift_loader_queue_url = https://sqs.us-west-2.amazonaws.com/792379844846/DasBackObject-loadtest
`
