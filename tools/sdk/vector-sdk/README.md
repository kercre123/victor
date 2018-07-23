# Anki Vector - Python SDK

## Getting Started

Before connecting, you will need:

* Vector's name: This is the name displayed on his face for BLE pairing after you double click while Vector is on the charger.
* Vcetor's IP Address: This may change, but may be gotten by placing Vector on the charger, double clicking the backpack, and raising then lowering the lift.
* Vector's public certificate: this may be retrieved after logging in to your account.

To start running the sdk examples,

* Install python3 if it's not available: `brew install python`

Either:

* Download the required python packages: `pip3 install -r requirements.txt`.

Or:

* Install the vector sdk using `setup.py`: `pip3 install -e .`.

## Tests

Note: The tests need to be updated to use grpc before they will be functional again.
