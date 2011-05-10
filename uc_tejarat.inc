<?php

/**
 * @file
 * Contains UC Tejarat Payment callbacks and helpers.
 */

/**
 * Menu callback for cart/tejarat/complete path, handling gateway response.
 */
function uc_tejarat_complete() {
  
}

/**
 * Internal helper to return a successfull request response.
 *
 * @return
 *   An array of request response or FALSE on failure.
 */
function _uc_tejarat_response() {
  if (isset($_POST['paymentId'], $_POST['referenceId'], $_POST['resultCode'])) {
    return array(
      'pid' => (int) $_POST['paymentId'],
      'rid' => check_plain($_POST['referenceId']),
      'result_code' => (int) $_POST['resultCode'],
    );
  }
  // Unexpected failure.
  return FALSE;
}
