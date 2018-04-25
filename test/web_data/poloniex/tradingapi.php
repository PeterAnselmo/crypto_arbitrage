<?php if($_POST['command'] == 'returnBalances'){ ?>
{"BTC":"1.00","BCH":"2.5","LTC":"3.25","ETH":"0.0000000"}
<?php } else if($_POST['command'] == 'returnFeeInfo'){ ?>
{"makerFee": "0.00140000", "takerFee": "0.00240000", "thirtyDayVolume": "612.00248891", "nextTier": "1200.00000000"}
<?php } ?>
