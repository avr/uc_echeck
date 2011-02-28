<?php

/**
 * Implementation of hook_recurring_info().
 */
function uc_echeck_recurring_info() {
  $gateway = uc_echeck_default_gateway();
  $items['echeck'] = array(
    'name' => t('eCheck'),
    'payment method' => 'echeck',
    'module' => 'uc_echeck',
    'fee handler' => $gateway,
    'process callback' => 'uc_echeck_recurring_invoke',
    'renew callback' => 'uc_echeck_recurring_invoke',
    'cancel callback' => 'uc_echeck_recurring_invoke',
  );
  return $items;
}

/**
 * eCheck process and renew.
 */
function uc_echeck_recurring_invoke($order, &$fee, $op) {
  // Invoke the correct payment method.
  return uc_recurring_invoke($fee->fee_handler, $op, array($order, &$fee));
}
