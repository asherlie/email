# email_notifier

provides a simple interface for sending emails

any code using this must be compiled with the flag -lcurl

to use a gmail account as a sender, 'less secure apps' must be enabled. this can be done [here](https://www.google.com/settings/security/lesssecureapps).

optionally uses [tkislan](http://github.com/tkislan)'s fork of https://github.com/adamvr/arduino-base64 for base64 conversion

# send

a lightweight email sending client written using email_notifier
