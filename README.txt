Description
===========
Ubercart Tejarat Bank Payment provides Ubercart 2.x a new "Payment Method" to process its online payments
through the Tejarat Bank Payment gateway (http://tejaratbank.ir/).

Requirements
============
1. A registered IP address on Tejarat Bank gateway.
2. A valid Merchant number and password combination provided by Parsian Bank.

Dependencies
============
- Ubercart 2.x and its uc_payment. (http://drupal.org/project/ubercart)
- SOAP Client (http://drupal.org/project/soapclient)

Installation
============
1. Install Ubercart 2.x if not yet installed.
  - Enable "Payment" module under "Ubercart - core (optional)" and all its dependencies.

2. Install & enable SOAP Client.
  - Go to admin/settings/soapclient.
  - Under "Active SOAP Library" check the "nuSOAP" option.
  - Leave other options as default.

3. Install and enable Ubercart Tejarat Payment.
  - Go to admin/store/settings/payment/edit/methods.
  - Check "Tejarat Bank" under "Payment methods" block.
  - Click on "Tejarat Bank settings" and fill the required information.
  - Leave other fields as default and save the configuration.

Author and Maintainer
=====================
Sepehr Lajevardi (http://drupal.org/user/668010)
