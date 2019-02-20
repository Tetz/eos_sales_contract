# Eos Sales Contract

**DISCLAIMER: This smart contract setup is not intended for production use, rather as an experimental setup 
to facilitate the understanding of EOS and the underlying principles. DO NOT USE this code in production or deploy it 
to the main network, as any EOS locked into the contract might be exposed to security risks. The code was not audited and 
should only be used for educational purposes.**

## Local setup
Tested on MacOs Mojave (10.14.2) and Linux Mint 18.3 Sylvia
## Prerequisites:

### 1. Operating System
You need to use either Linux or MacOs to interact with the contract as the eosio software currently does not support the operating system Windows. It is possible to enable linux as a subsystem in Windows. 
However, the following guide is aimed at Linux and MacOs systems. 
If you are using Windows you can get started here (https://medium.com/@blockgenic/eosio-single-node-testnet-setup-on-windows-ae7a59900e69).

### 2. NPM
You need to have npm installed to work with this implementation.  
It is recommended to use a node version manager such as nvm  (https://github.com/creationix/nvm) to install npm

### 3. Keosd, cleos and nodeos
Please follow step 1.2 of the developer tutorial of EOS and install eosio, keosd, cleos and nodeos. Link: https://developers.eos.io/eosio-home/docs/setting-up-your-environment  
Versions:  
- cleos: client Build version: d4ffb4eb
- nodeos: v1.5.1
- keosd: v1.5.1  
Versions that differ from the aforementioned, can cause problems in the workflow. As there might be some breaking changes.  

**Do not create any other configurations such as the default wallet and test accounts from step 1.6 and 1.7 as they will be created automatically in the next step**

### 4. Clone this repository into a new folder/directory

### 5. Install all necessary dependencies through running the install script
From the root directory (eos_sales_contract) open a terminal and run: 
`npm install` 
The complete script should run without problems (no error messages), otherwise there will be problems later on! (yellow output is normal, as this just gives you the hint that the transactions are only executed locally)
(**Remark: This command will install all dependencies and it will run a setup script which makes other scripts executable, these necessary for the tests, furthermore it will create a default wallet. The password for the default wallet will be stored in scripts/pw.txt you only need it if you want to unlock it manually)** 
## Start the Tests
**Only after all previous steps (prerequisites 1-5 and npm install)**  
From the root directory (eos_sales_contract) run:  
`npm test`

## Start the Frontend

**Only after all previous steps (prerequisites 1-5 and npm install)**  
From the root directory (eos_sales_contract) run:  
`npm start`  
This will initialize a new eos chain (nodeos) and delete any previous data. 
The contract is now ready for interaction. But before, it is necessary to install scatter.

### Install Scatter  

To interact with the smart contract on the EOS Blockchain download the wallet provider Scatter (https://get-scatter.com/). Please follow the next steps:
- Start Scatter after downloading it
- Create a new wallet
- Import the private key for interaction (private key: 5K8ghcBf9TpPAWdxHDUejqcxBWrQAzkj5D5FWHe13nmNJmWhH9k)
- After importing the private key scatter should show you 5 accounts (seller, buyer, intermed, salescon, random)
- Go into the settings of Scatter and add a new network (this is under the section "danger zone")
  - Delete all currently existing networks
  - Afterwards add a new network (Hint: After deletion of the networks it might be necessary to leave the options and come back again, as there are some 
  refreshing problems)
  - Add the following configs for the network: 
    - Name: Testnet
    - Host: 127.0.0.1
    - Protocol: http
    - Port: 8888
    - chainId: cf057bbfb72640471fd910bcb67639c22df9f92470936cddc1ade0e2f2e7dc4f

Now open http://localhost:8082 and you can interact with the contract.

### Intended steps for interaction: 
- Set Item
- Pay Item
- Received Item
- Withdraw

Everytime a button is clicked, Scatter will ask you for permission.



