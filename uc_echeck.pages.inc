<?php

/**
 * @file
 * Credit menu items.
 *
 */

// Prints the contents of the CVV information popup window.
function uc_echeck_info() {
  $output = '';
  // $output .= '<b>'. t('What is the CVV?') .'</b><p>'. t('CVV stands for Card Verification Value. This number is used as a security feature to protect you from credit card fraud.  Finding the number on your card is a very simple process.  Just follow the directions below.') .'</p>';
  $output .= '<br /><p><img src="'
          . base_path() . drupal_get_path('module', 'uc_echeck')
          .'/images/checkhelp.jpg" align="left" />'
          . t('The CVV for these cards is found on the back side of the card.  It is only the last three digits on the far right of the signature panel box.') .'</p>';

  $output .= '<p><div align="right"><input type="button" onclick="self.close();" '
          .'value="'. t('Close this window') .'" /></div></p>';

  print $output;
  exit();
}