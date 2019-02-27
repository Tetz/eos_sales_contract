#include <eosiolib/eosio.hpp>
#include "./salescon.hpp"

using namespace eosio;
using namespace std;

/**
 * initialization
 * @params { name seller, name buyer, name intermediator }
 * @conditions none 
 * @actions initializes config (sets seller, buyer, intermediator) and agreement struct,
 * caller is the seller of the contract
 * @eos action
 */
void salescon::init(name seller, name buyer, name intermediator)
{
  auto iterator = _config.find(0);
  eosio_assert(iterator == _config.end(), "Contract already initialized");
  _config.emplace(seller, [&](auto &row) {
    row.key = 0;
    row.seller = seller;
    row.buyer = buyer;
    row.intermediator = intermediator;
    row.balance = asset(0, EOS_SYMBOL);
    row.contractIsClosed = false;
    row.itemSet = false;
    row.buyerIsPaidBack = false;
    row.contractRetracted = false;
    row.itemReceived = false;
    row.itemPaid = false;
  });
  auto agreeIt = _agreement.find(0);
  eosio_assert(agreeIt == _agreement.end(), "Contract already initialized");
  _agreement.emplace(seller, [&](auto &row) {
    row.key = 0;
    row.sellerRetract = false;
    row.buyerRetract = false;
    row.intermediatorRetract = false;
  });
  setBalance(asset(0, EOS_SYMBOL), seller);
}

/**
 * setItem
 * @params { string itemName, uint64_t itemPrice}
 * @conditions assertInitialization, only seller (see config), itemIsSet == false
 * @actions sets the item in the multi index table { item }
 * @eos action
 */
void salescon::setitem(std::string itemName, asset itemPrice)
{

  name seller = getSeller();
  require_auth(seller);
  assertInitialized();
  eosio_assert(itemName != "", "Item name must not be null");
  eosio_assert(itemPrice.symbol.is_valid(), "Invalid quantity");
  eosio_assert(itemPrice.amount > 0, "Only positive quantities can be transferred");
  eosio_assert(itemPrice.symbol == EOS_SYMBOL, "Asset must be of type EOS and with exact 4 decimal places");
  // This is necessary to ensure that the amount is in the right decimal places 

  auto iterator = _item.find(0);
  eosio_assert(iterator == _item.end(), "Item already set");
  _item.emplace(seller, [&](auto &row) {
    row.key = 0;
    row.itemName = itemName;
    row.itemPrice = itemPrice;
  });
  setItemIsSetFlag(true, seller);
}

/**
 * itemreceived
 * @params {}
 * @conditions itemPaid == true, only by buyer
 * @actions sets itemReceived = true
 * @eos action
 */
void salescon::itemreceived()
{
  assertItemPaid();
  name buyer = getBuyer();
  require_auth(buyer);
  setItemReceivedFlag(true, buyer);
}

/**
 * transfer
 * @params { name from, name to, asset quantity, string memo }, same parameters as from the eosio.token transfer function
 * @conditions quantity.amount == itemPrice, only buyer, contract is recipient, correct token, only positive amount, contract is intact and not retracted
 * @actions recognizes a transfer of EOS (eosio.token contract) to this contract address, sets itemIsPaid = true and balance = itemPrice
 * @eos action
 */
void salescon::transfer(name from, name to, asset quantity, string memo)
{
  if (from == _self)
  {
    return;
  }
  name buyer = getBuyer();

  // Necessary asserts
  assertContractClosedStatus(false);
  assertItemSet();
  assertRetractStatus(false);
  assertPriceEqualsValue(quantity.amount);
  eosio_assert(from == buyer, "Transfer must come from buyer");
  eosio_assert(to == _self, "Contract was not the recipient");
  eosio_assert(quantity.symbol.is_valid(), "Invalid quantity");
  eosio_assert(quantity.amount > 0, "Only positive quantities can be transferred");
  eosio_assert(quantity.symbol == EOS_SYMBOL, "Only transfer from EOS token and exact 4 decimal places possible");

  // If everything is valid set item to paid
  itempaid(_self);
}


/**
 * withdraw
 * @params { name to }
 * @conditions itemPaid == true, only by seller or buyer (dispute), contract intact
 * @actions lets the seller withdraw their money, in case of dispute either buyer or seller can withdraw
 * @eos action
 */
void salescon::withdraw(name to)
{
  assertContractClosedStatus(false);
  auto config = getConfig();
  if (config.buyerIsPaidBack)
  {
    require_auth(getBuyer());
  }
  else
  {
    require_auth(getSeller());
  }
  if (getConfig().contractRetracted == false) {
    assertItemReceived();   
  }
  require_auth(to);
  auto price = getPrice();
  setContractClosedStatus(true, to);
  setBalance(asset(0, EOS_SYMBOL), to);
  sendTokens(to, price);
}

/**
 * retract
 * @params { name retractor }
 * @conditions contractIsClosed == false, contractRetracted == false, only buyer, seller and intermed can retract contract
 * buyer only if seller does not retract and seller only if buyer does not retract
 * @actions two parties need to retract, then contractIsClosed = true (if no balance in contract) and contractRetracted = true
 * @eos action
 */
void salescon::retract(name retractor)
{
  name seller = getSeller();
  name buyer = getBuyer();
  assertContractClosedStatus(false);
  assertRetractStatus(false);

  auto agreement = getAgreement();
  if (retractor == buyer)
  {
    require_auth(buyer);
    eosio_assert(!agreement.sellerRetract, "Can not retract, because seller already retracted");
    buyerRetract(buyer);
  }
  else if (retractor == seller){
    require_auth(seller);
    eosio_assert(!agreement.buyerRetract, "Can not retract, because buyer already retracted");
    sellerRetract(seller);
  } else {
    eosio_assert(true == false, "Caller does not have the permission to call this method");
  }
}

/**
 * finalretract
 * @params { bool buyerIsRight } indicates if the buyer is ruled right in dispute
 * @conditions contractIsClosed == false, contractRetracted == false, only intermed can finalize retraction
 * @actions intermed can finalize the retraction in favor of one party
 * @eos action
 */
void salescon::finalretract(bool buyerIsRight)
{
  name intermediator = getIntermediator();
  require_auth(intermediator);
  assertContractClosedStatus(false);
  assertRetractStatus(false);
  assertMarkedAsRetracted();
  intermediatorRetract(intermediator);
  configureRetractedState(buyerIsRight);
}

/**
 * changeseller
 * @params { name newSeller }
 * @conditions only by seller, contractRetracted == false, contract is initialized
 * @actions seller wants to change the seller account of the contract
 * @eos action
 */
void salescon::changeseller(name newSeller) {
  assertRetractStatus(false);
  name seller = getSeller();
  require_auth(seller);
  auto configIt = _config.find(0);
  eosio_assert(configIt != _config.end(), "changeseller: Contract must be initialized");
  _config.modify(configIt, seller, [&](auto &row) {
     row.seller = newSeller;
  });
}

void salescon::itempaid(name payer)
{
  setItemIsPaidFlag(true, payer);
  asset price = getPrice();
  setBalance(price, payer);
  print("Item is paid");
}

void salescon::setItemIsSetFlag(bool value, name payer)
{
  auto iterator = _config.find(0);
  if (iterator != _config.end())
  {
    _config.modify(iterator, payer, [&](auto &row) {
      row.itemSet = value;
    });
  }
}

void salescon::setItemIsPaidFlag(bool value, name payer)
{
  auto iterator = _config.find(0);
  if (iterator != _config.end())
  {
    _config.modify(iterator, payer, [&](auto &row) {
      row.itemPaid = value;
    });
  }
}

void salescon::setItemReceivedFlag(bool value, name payer)
{
  auto iterator = _config.find(0);
  if (iterator != _config.end())
  {
    _config.modify(iterator, payer, [&](auto &row) {
      row.itemReceived = value;
    });
  }
}

void salescon::setContractClosedStatus(bool value, name payer)
{
  auto iterator = _config.find(0);
  if (iterator != _config.end())
  {
    _config.modify(iterator, payer, [&](auto &row) {
      row.contractIsClosed = value;
    });
  }
}

void salescon::setRetractStatus(bool value, name payer)
{
  auto iterator = _config.find(0);
  if (iterator != _config.end())
  {
    _config.modify(iterator, payer, [&](auto &row) {
      row.contractRetracted = value;
    });
  }
}

void salescon::setBuyerPaidBack(bool value, name payer)
{
  auto iterator = _config.find(0);
  if (iterator != _config.end())
  {
    _config.modify(iterator, payer, [&](auto &row) {
      row.buyerIsPaidBack = value;
    });
  }
}

void salescon::setBalance(asset value, name payer)
{
  auto iterator = _config.find(0);
  if (iterator != _config.end())
  {
    _config.modify(iterator, payer, [&](auto &row) {
      row.balance = value;
    });
  }
}

void salescon::configureRetractedState(bool buyerIsRight)
{
  name intermediator = getIntermediator();
  auto config = getConfig();
  if (config.balance.amount == 0)
  {
    setContractClosedStatus(true, intermediator);
  }
  else
  {
    setBuyerPaidBack(buyerIsRight, intermediator);
  }
  setRetractStatus(true, intermediator);
  
}

void salescon::buyerRetract(name buyer)
{
  auto iterator = _agreement.find(0);
  _agreement.modify(iterator, buyer, [&](auto &row) {
    row.buyerRetract = true;
  });
}

void salescon::sellerRetract(name seller)
{
  auto iterator = _agreement.find(0);
  _agreement.modify(iterator, seller, [&](auto &row) {
    row.sellerRetract = true;
  });
}

void salescon::intermediatorRetract(name intermediator)
{
  auto iterator = _agreement.find(0);
  _agreement.modify(iterator, intermediator, [&](auto &row) {
    row.intermediatorRetract = true;
  });
}

void salescon::sendTokens(name to, asset price)
{
  action(
      permission_level{get_self(), "active"_n},
      name("eosio.token"),
      "transfer"_n,
      std::make_tuple(get_self(), to, price, std::string("")))
      .send();
  setBalance(asset(0, EOS_SYMBOL), to);
}

void salescon::assertInitialized()
{
  auto iterator = _config.find(0);
  eosio_assert(iterator != _config.end(), "assert initialized: Contract must be initialized!");
}

void salescon::assertItemSet()
{
  auto config = getConfig();
  eosio_assert(config.itemSet == true, "assertItemSet: Item was not marked as set");
}

void salescon::assertItemReceived()
{
  auto config = getConfig();
  eosio_assert(config.itemReceived == true, "assertItemReceived: Item was not marked as received");
}

void salescon::assertItemPaid()
{
  auto config = getConfig();
  eosio_assert(config.itemPaid == true, "assertItemPaid: Item was not paid");
}

void salescon::assertPriceEqualsValue(uint64_t value)
{
  auto iterator = _item.find(0);
  eosio_assert((*iterator).itemPrice.amount == value, "assertPriceEqualsValue: Transfer value must be equal to price");
}

void salescon::assertRetractStatus(bool status)
{
  auto config = getConfig();
  status ? eosio_assert(config.contractRetracted == status, "assertRetractStatus: Contract must be retracted") : eosio_assert(config.contractRetracted == status, "Contract must not be retracted");
}

void salescon::assertContractClosedStatus(bool status)
{
  auto config = getConfig();
  status ? eosio_assert(config.contractIsClosed == status, "assertContractClosedStatus: Contract should be closed") : eosio_assert(config.contractIsClosed == status, "Contract is closed");
}

void salescon::assertMarkedAsRetracted()
{
  auto iterator = _agreement.find(0);
  eosio_assert(iterator != _agreement.end(), "assertMarkedAsRetracted: Agreement not initialized");
  const auto &agree = *iterator;
  eosio_assert(agree.sellerRetract || agree.buyerRetract, "Contract is not marked as retracted by buyer or seller");
}

name salescon::getSeller()
{
  auto iterator = _config.find(0);
  eosio_assert(iterator != _config.end(), "Tried to call getSeller(), but contract is not initialized");
  return (*iterator).seller;
}

name salescon::getBuyer()
{
  auto iterator = _config.find(0);
  eosio_assert(iterator != _config.end(), "Tried to call getBuyer(), but contract is not initialized");
  return (*iterator).buyer;
}

name salescon::getIntermediator()
{
  auto iterator = _config.find(0);
  eosio_assert(iterator != _config.end(), "Tried to call getIntermediator(), but contract is not initialized");
  return (*iterator).intermediator;
}

asset salescon::getPrice()
{
  auto iterator = _item.find(0);
  eosio_assert(iterator != _item.end(), "Tried to call getPrice(), but item is not set");
  return (*iterator).itemPrice;
}

eosio::salescon::agreestruct salescon::getAgreement()
{
  auto agreeIt = _agreement.find(0);
  eosio_assert(agreeIt != _agreement.end(), "getAgreement(): Contract must be initialized");
  return (*agreeIt);
}

eosio::salescon::configstruct salescon::getConfig()
{
  auto configIt = _config.find(0);
  eosio_assert(configIt != _config.end(), "getConfig(): Contract must be initialized");
  return (*configIt);
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
  if (code == name("eosio.token").value && action == name("transfer").value)
  {
    execute_action(
        name(receiver), name(receiver), &salescon::transfer);
  }
  else if (code == receiver)
  {
    switch (action)
    {
      EOSIO_DISPATCH_HELPER(salescon, (setitem)(init)(itemreceived)(withdraw)(retract)(finalretract)(changeseller))
    }
  }
}
