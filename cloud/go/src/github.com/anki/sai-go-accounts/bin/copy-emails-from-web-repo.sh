#!/usr/bin/env bash
# Param 1 = path to sai-web-emails
# Param 2 = "s[ummary]" or "d[iff]" or "c[opy]"

WEBPROJROOT=$1
ACCOUNTSEMAILS="$PWD/.."
ACTION=${2:0:1}

echo "Action: $ACTION"

echo "$ACCOUNTSEMAILS"

if [ ! -d "$WEBPROJROOT" ]; then
	echo "Path to (and including) sai-web-emails directory does not exist"
	exit 1
fi

if [ ! -f "$WEBPROJROOT/emails-mjml/corporate/emails/ams/account-update/message-DE.html" ]; then
	echo "Path to sai-web-emails does not appear to have the right files: $WEBPROJROOT/emails-mjml/corporate/emails/ams/account-update/message-DE.html"
	exit 2
fi

echo "OK sai-web-emails directory: $WEBPROJROOT"

if [ ! -f "$ACCOUNTSEMAILS/email/templates/en/email_parental_post_verification.html" ]; then
	ACCOUNTSEMAILS="$PWD"
	if [ ! -f "$ACCOUNTSEMAILS/email/templates/en/email_parental_post_verification.html" ]; then
		echo "Script does not appear to be run from the root of sai-go-accounts: $PWD"
		exit 3
	fi
fi

FILETYPE_MAP=(
 "account-update:email_verification_account_updated"
 "activate-account:email_verification"
 "activate-account-14day-reminder:email_verification_14day_reminder"
 "activate-account-14day-reminder-under-13:email_verification_under13_14day_reminder"
 "activate-account-7day-reminder:email_verification_7day_reminder"
 "activate-account-7day-reminder-under-13:email_verification_under13_7day_reminder"
 "activate-under-13:email_verification_under13"
 "parental-post-verification:email_parental_post_verification"
 "password-request:email_forgotpassword"
)

getHashKey() {
	declare -a hash=("${!1}")
	local key
	local lookup=$2

	for key in "${hash[@]}" ; do
		KEY=${key%%:*}
		VALUE=${key#*:}
		if [[ $KEY == $lookup ]]
		then
			echo $VALUE
		fi
	done
}

act () {
	FILE=$1
	FULLFILE="$WEBPROJROOT/$FILE"
	echo "Working on $FILE"

	IFS='/' read -ra PARTS <<< "$FILE"
	FILENAME=${PARTS[${#PARTS[@]}-1]} # Old bash cannot do -1
	MESSAGETYPE=${PARTS[${#PARTS[@]}-2]}
	echo $MESSAGETYPE

	IFS='-' read -ra FAZ <<< "$FILENAME"
	FNAMEA=${FAZ[${#FAZ[@]}-1]}
	IFS='.' read -ra FNAMEB <<< "$FNAMEA"
	COUNTRY=${FNAMEB[0]}
        TYPE=${FNAMEB[1]}
	echo "TYPE $TYPE"

        COUNTRYLC="$(tr [A-Z] [a-z] <<< "$COUNTRY")"
	COUNTRYLC="$(tr [A-Z] [a-z] <<< "$COUNTRY")"
	COUNTRYLC="${COUNTRYLC/us/en}"
	echo "COUNTRY: $COUNTRYLC"
	
	RESMTYPE=$(getHashKey FILETYPE_MAP[@] $MESSAGETYPE)
	echo "RESULT MESSAGE TYPE: $RESMTYPE"

	RESFILE="$ACCOUNTSEMAILS/email/templates/$COUNTRYLC/$RESMTYPE.$TYPE"
	echo "Result Filename: $RESFILE"

	ACTIONTAKEN=0

	if [[ $ACTION == "s" ]]; then
		ACTIONTAKEN=1
		echo "SUMMARY $FULLFILE => $RESFILE"
	fi
	
	if [[ $ACTION == "d" ]]; then
		ACTIONTAKEN=1
		echo "DIFF START $FULLFILE => $RESFILE"
		diff $FULLFILE $RESFILE
		echo "DIFF DONE $FULLFILE => $RESFILE"
	fi

	# | sed '/During the creation of this Anki Account/d' | sed '/Unsubscribing has no effect/d'
	# Eine Abmeldung hat keinen   |    manage_url_20738
	if [[ $ACTION == "c" ]]; then
		ACTIONTAKEN=1
		if grep -q '{{ define "content" }}' $FULLFILE; then
			cat $FULLFILE | sed '/During the creation of this Anki Account/d' | sed '/Unsubscribing has no effect/d' | sed '/Eine Abmeldung hat keinen/d' | sed '/manage_url_20738/d'  > $RESFILE
		else
			echo '{{ define "content" }}' > $RESFILE
			cat $FULLFILE | sed '/During the creation of this Anki Account/d' | sed '/Unsubscribing has no effect/d' | sed '/Eine Abmeldung hat keinen/d' | sed '/manage_url_20738/d' >> $RESFILE
			echo '{{ end }}' >> $RESFILE	
		fi
	fi

	if [[ $ACTIONTAKEN == 0 ]]; then
		echo "No proper action specified"
		exit 10
	fi
}

act "emails-mjml/corporate/emails/ams/account-update/message-DE.html"
act "emails-mjml/corporate/emails/ams/account-update/message-FR.html"
act "emails-mjml/corporate/emails/ams/account-update/message-US.html"
act "emails-mjml/corporate/emails/ams/account-update/text-DE.txt"
act "emails-mjml/corporate/emails/ams/account-update/text-EN.txt"
act "emails-mjml/corporate/emails/ams/account-update/text-FR.txt"
act "emails-mjml/corporate/emails/ams/activate-account/message-DE.html"
act "emails-mjml/corporate/emails/ams/activate-account/message-FR.html"
act "emails-mjml/corporate/emails/ams/activate-account/message-US.html"
act "emails-mjml/corporate/emails/ams/activate-account/text-DE.txt"
act "emails-mjml/corporate/emails/ams/activate-account/text-EN.txt"
act "emails-mjml/corporate/emails/ams/activate-account/text-FR.txt"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder/message-DE.html"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder/message-FR.html"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder/message-US.html"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder/text-DE.txt"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder/text-EN.txt"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder/text-FR.txt"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder-under-13/message-DE.html"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder-under-13/message-FR.html"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder-under-13/message-US.html"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder-under-13/text-DE.txt"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder-under-13/text-EN.txt"
act "emails-mjml/corporate/emails/ams/activate-account-14day-reminder-under-13/text-FR.txt"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder/message-DE.html"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder/message-FR.html"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder/message-US.html"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder/text-DE.txt"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder/text-EN.txt"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder/text-FR.txt"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder-under-13/message-DE.html"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder-under-13/message-FR.html"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder-under-13/message-US.html"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder-under-13/text-DE.txt"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder-under-13/text-EN.txt"
act "emails-mjml/corporate/emails/ams/activate-account-7day-reminder-under-13/text-FR.txt"
act "emails-mjml/corporate/emails/ams/activate-under-13/message-DE.html"
act "emails-mjml/corporate/emails/ams/activate-under-13/message-FR.html"
act "emails-mjml/corporate/emails/ams/activate-under-13/message-US.html"
act "emails-mjml/corporate/emails/ams/activate-under-13/text-DE.txt"
act "emails-mjml/corporate/emails/ams/activate-under-13/text-EN.txt"
act "emails-mjml/corporate/emails/ams/activate-under-13/text-FR.txt"
act "emails-mjml/corporate/emails/ams/parental-post-verification/message-DE.html"
act "emails-mjml/corporate/emails/ams/parental-post-verification/message-FR.html"
act "emails-mjml/corporate/emails/ams/parental-post-verification/message-US.html"
act "emails-mjml/corporate/emails/ams/parental-post-verification/text-DE.txt"
act "emails-mjml/corporate/emails/ams/parental-post-verification/text-EN.txt"
act "emails-mjml/corporate/emails/ams/parental-post-verification/text-FR.txt"
act "emails-mjml/corporate/emails/ams/password-request/message-DE.html"
act "emails-mjml/corporate/emails/ams/password-request/message-FR.html"
act "emails-mjml/corporate/emails/ams/password-request/message-US.html"
act "emails-mjml/corporate/emails/ams/password-request/text-DE.txt"
act "emails-mjml/corporate/emails/ams/password-request/text-EN.txt"
act "emails-mjml/corporate/emails/ams/password-request/text-FR.txt"
exit

