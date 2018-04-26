<?php
$balances = array(
    "BTC"=>"1.00",
    "XRP"=>"50.0",
    "BCH"=>"2.5",
    "LTC"=>"3.25",
    "ETH"=>"0.0000000")
?>
<?php if($_POST['command'] == 'returnBalances'){ ?>
<?php echo json_encode($balances) ?>
<?php } else if($_POST['command'] == 'returnFeeInfo'){ ?>
{"makerFee": "0.00140000", "takerFee": "0.00250000", "thirtyDayVolume": "612.00248891", "nextTier": "1200.00000000"}
<?php 

//command=sell&currencyPair=BTC_XRP&rate=0.00009277&amount=179.69999695&immediateOrCancel=1&nonce=1524721917678
} else if ($_POST['command'] == 'sell'){
    if($_POST['rate'] > 0.00009979){
        $response = array(
            "orderNumber"=>"144484516971",
            "resultingTrades"=>[],
            "amountUnfilled"=>$_POST["amount"]);
        echo json_encode($response);
    } else {
        //command=sell&currencyPair=BTC_XRP&rate=0.00008990&amount=179.69999695&immediateOrCancel=1&nonce=1524724856176
        echo '{"orderNumber":"144492489990","resultingTrades":[{"amount":"179.69999695","date":"2018-04-26 06:40:57","rate":"0.00008996","total":"0.01616581","tradeID":"21662264","type":"sell"}],"amountUnfilled":"0.00000000"}';

    }
} else {
    echo var_dump($_POST);
}
