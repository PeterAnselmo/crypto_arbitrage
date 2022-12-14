<?php
$balances = array(
    "BTC"=>array("available"=>"0.00315", "btcValue"=>"0.003"),
    "XRP"=>array("available"=>"50.0", "btcValue"=>"0.003"),
    "BCH"=>array("available"=>"2.5", "btcValue"=>"0.003"),
    "GAS"=>array("available"=>"1.00", "btcValue"=>"0.003"),
    "XMR"=>array("available"=>"0.10", "btcValue"=>"0.003"),
    "MAID"=>array("available"=>"2.00", "btcValue"=>"0.003"),
    "ZEC"=>array("available"=>"0.00", "btcValue"=>"0.003"),
    "USDT"=>array("available"=>"30.00", "btcValue"=>"0.003"),
    "ETH"=>array("available"=>"0.0440000", "btcValue"=>"0.003"),
);
$market_rates = array(
    "BTC_ETH" => 0.07410000,
    "ETH_GAS" => 0.04385234,
    "BTC_GAS" => 0.00421197);

if($_POST['command'] == 'returnCompleteBalances'){
    echo json_encode($balances);

} else if($_POST['command'] == 'returnFeeInfo'){
    echo '{"makerFee": "0.00140000", "takerFee": "0.00250000", "thirtyDayVolume": "612.00248891", "nextTier": "1200.00000000"}';

//command=sell&currencyPair=BTC_XRP&rate=0.00009277&amount=179.69999695&immediateOrCancel=1&nonce=1524721917678
} else if ($_POST['command'] == 'sell'){
    if($_POST['rate'] <= $market_rates[$_POST['currencyPair']]){
        //command=sell&currencyPair=BTC_XRP&rate=0.00008990&amount=179.69999695&immediateOrCancel=1&nonce=1524724856176
        $response = array(
            "orderNumber" => "270275494074",
            "resultingTrades"=>array(array(
                "amount"=> $_POST['amount'],
                "date"=>"2018-04-26 19:52:58",
                "rate"=> sprintf('%0.8f',$market_rates[$_POST['currencyPair']]),
                "total"=> strval($market_rates[$_POST['currencyPair']] * $_POST['amount']),
                "tradeID"=>"16843227",
                "type"=>$_POST['command'])),
            "amountUnfilled"=>"0.00000000");
    } else {
        $response = array(
            "orderNumber"=>"144484516971",
            "resultingTrades"=>[],
            "amountUnfilled"=>$_POST["amount"]);
    }
    echo json_encode($response);

//command=buy&currencyPair=BTC_ZEC&rate=0.03009176&amount=0.34958404&immediateOrCancel=1&nonce=1524771128271
} else if ($_POST['command'] == 'buy'){
    if($_POST['rate'] >= $market_rates[$_POST['currencyPair']]){
        $response = array(
            "orderNumber" => "270275494074",
            "resultingTrades"=>array(array(
                "amount"=> $_POST['amount'],
                "date"=>"2018-04-26 19:52:58",
                "rate"=> sprintf('%0.8f',$market_rates[$_POST['currencyPair']]),
                "total"=> strval($market_rates[$_POST['currencyPair']] * $_POST['amount']),
                "tradeID"=>"16843227",
                "type"=>$_POST['command'])),
            "amountUnfilled"=>"0.00000000");
    } else {
        $response = array(
            "orderNumber"=>"119971507579",
            "resultingTrades"=>[],
            "amountUnfilled"=>$_POST["amount"]);
    }
    echo json_encode($response);
} else {
    echo var_dump($_POST);
}
