<?php
$balances = array(
    "BCH"=>array("available"=>"0.03365079","onOrders"=>"0.00000000","btcValue"=>"0.00489725"),
    "BCN"=>array("available"=>"4223.64411992","onOrders"=>"0.00000000","btcValue"=>"0.00451929"),
    "BTC"=>array("available"=>"0.00503993","onOrders"=>"0.00000000","btcValue"=>"0.00503993"),
    "CVC"=>array("available"=>"131.78159881","onOrders"=>"0.00000000","btcValue"=>"0.00499056"),
    "DASH"=>array("available"=>"0.10575112","onOrders"=>"0.00000000","btcValue"=>"0.00484083"),
    "ETC"=>array("available"=>"2.37878751","onOrders"=>"0.00000000","btcValue"=>"0.00496148"),
    "ETH"=>array("available"=>"0.06194710","onOrders"=>"0.00000000","btcValue"=>"0.00510861"),
    "GAS"=>array("available"=>"1.69513468","onOrders"=>"0.00000000","btcValue"=>"0.00469754"),
    "GNO"=>array("available"=>"0.51575145","onOrders"=>"0.00000000","btcValue"=>"0.00551596"),
    "GNT"=>array("available"=>"70.92220828","onOrders"=>"0.00000000","btcValue"=>"0.00441490"),
    "LSK"=>array("available"=>"4.09212541","onOrders"=>"0.00000000","btcValue"=>"0.00525719"),
    "LTC"=>array("available"=>"0.32558838","onOrders"=>"0.00000000","btcValue"=>"0.00521919"),
    "MAID"=>array("available"=>"104.47186680","onOrders"=>"0.00000000","btcValue"=>"0.00465108"),
    "NXT"=>array("available"=>"245.42446815","onOrders"=>"0.00000000","btcValue"=>"0.00481277"),
    "OMG"=>array("available"=>"3.06126711","onOrders"=>"0.00000000","btcValue"=>"0.00452553"),
    "REP"=>array("available"=>"0.83584316","onOrders"=>"0.00000000","btcValue"=>"0.00505863"),
    "STEEM"=>array("available"=>"15.58560634","onOrders"=>"0.00000000","btcValue"=>"0.00469485"),
    "STR"=>array("available"=>"132.36160469","onOrders"=>"0.00000000","btcValue"=>"0.00491458"),
    "USDT"=>array("available"=>"45.36450403","onOrders"=>"0.00000000","btcValue"=>"0.00000000"),
    "XMR"=>array("available"=>"0.22085232","onOrders"=>"0.00000000","btcValue"=>"0.00513201"),
    "XRP"=>array("available"=>"61.94087190","onOrders"=>"0.00000000","btcValue"=>"0.00497632"),
    "ZEC"=>array("available"=>"0.13141881","onOrders"=>"0.00000000","btcValue"=>"0.00510463"),
    "ZRX"=>array("available"=>"30.89640527","onOrders"=>"0.00000000","btcValue"=>"0.00485845")
);
$market_rates = array(
    "BTC_BCN" => 0.00000110,
    "BTC_XMR" => 0.02418375,
    "XMR_BCN" => 0.00004489);

if($_POST['command'] == 'returnCompleteBalances'){
    echo json_encode($balances);

} else if($_POST['command'] == 'returnFeeInfo'){
    echo '{"makerFee": "0.00100000", "takerFee": "0.00200000", "thirtyDayVolume": "612.00248891", "nextTier": "1200.00000000"}';

//command=sell&currencyPair=BTC_XRP&rate=0.00009277&amount=179.69999695&immediateOrCancel=1&nonce=1524721917678
} else if ($_POST['command'] == 'sell' || $_POST['command'] == 'buy'){
    $total = sprintf('%0.8f',$_POST['rate'] * $_POST['amount']);
    if ($_POST['command'] == 'sell'){
        list($buy, $sell) = explode("_", $_POST['currencyPair']);
        if($_POST['amount'] > $balances[$sell]){
            $response = array(
                "error"=>"Not Enough $sell"
            );
        } else if($_POST['rate'] <= $market_rates[$_POST['currencyPair']]){
            //command=sell&currencyPair=BTC_XRP&rate=0.00008990&amount=179.69999695&immediateOrCancel=1&nonce=1524724856176
            $response = array(
                "orderNumber" => "270275494074",
                "resultingTrades"=>array(array(
                    "amount"=> $_POST['amount'],
                    "date"=>"2018-04-26 19:52:58",
                    "rate"=> sprintf('%0.8f',$market_rates[$_POST['currencyPair']]),
                    "total"=> strval($total),
                    "tradeID"=>"16843227",
                    "type"=>$_POST['command'])),
                "amountUnfilled"=>"0.00000000");
        } else {
            $response = array(
                "orderNumber"=>"144484516971",
                "resultingTrades"=>[],
                "amountUnfilled"=>$_POST["amount"]);
        }

    //command=buy&currencyPair=BTC_ZEC&rate=0.03009176&amount=0.34958404&immediateOrCancel=1&nonce=1524771128271
    } else if ($_POST['command'] == 'buy'){
        list($sell, $buy) = explode("_", $_POST['currencyPair']);
        if($total > $balances[$sell]){
            $response = array(
                "error"=>"Not Enough $sell"
            );
        } else if($_POST['rate'] >= $market_rates[$_POST['currencyPair']]){
            $response = array(
                "orderNumber" => "270275494074",
                "resultingTrades"=>array(array(
                    "amount"=> $_POST['amount'],
                    "date"=>"2018-04-26 19:52:58",
                    "rate"=> sprintf('%0.8f',$market_rates[$_POST['currencyPair']]),
                    "total"=> strval($total),
                    "tradeID"=>"16843227",
                    "type"=>$_POST['command'])),
                "amountUnfilled"=>"0.00000000");
        } else {
            $response = array(
                "orderNumber"=>"119971507579",
                "resultingTrades"=>[],
                "amountUnfilled"=>$_POST["amount"]);
        }
    }
    echo json_encode($response);
} else {
    echo var_dump($_POST);
}?>
