# Springboard Developer

This module aims to help Springboard developers with various, menial tasks. Features supported so far:
- Reset Springboard submission data: this will reset all submission related data, including donations, webform submissions, stored cards, and users (excluding the admin user).
- Quick enabling/disabling of payment gateways.
- Auto fill payment gateways: log into LastPass and automatically fill each supported payment gateway's configuration options using values from LastPass.
- Auto configure payment gateways: automatically configures payment gateways to use the developer options.

# Requirements

In order to use the LastPass functionality, you will need the [phpseclib library](https://github.com/phpseclib/phpseclib) (place in `sites/all/libraries/phpseclib`) and the [LastPass-PHP library](https://github.com/robertromore/lastpass-php) (place in `sites/all/libraries/lastpass-php`).

# Usage

Use the module by accessing `springboard/springboard-developer/`.
