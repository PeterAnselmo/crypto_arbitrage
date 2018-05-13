<?php
$balances = array(
    "BTC"=>"0.00309392",
    "XMR"=>"0.11874890",
    "BCN"=>"4156.25000000");
$market_rates = array(
    "BTC_BCN" => 0.00000110,
    "BTC_XMR" => 0.02418375,
    "XMR_BCN" => 0.00004489);

if($_POST['command'] == 'returnBalances'){
    echo json_encode($balances);

} else if($_POST['command'] == 'returnFeeInfo'){
    echo '{"makerFee": "0.00140000", "takerFee": "0.00250000", "thirtyDayVolume": "612.00248891", "nextTier": "1200.00000000"}';

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
