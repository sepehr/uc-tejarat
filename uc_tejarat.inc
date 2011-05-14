<?php

/**
 * @file
 * Contains UC Tejarat Payment callbacks and helpers.
 *
 * @todo
 *   - Avoid enquiry request flood.
 *   - Test out the module on a production server with a registered IP.
 *   - Drop the nuSOAP library and implement the SOAPClient module's API. It's
 *     incompatible with Tejarat Bank's legacy webservices!
 */

/**
 * Indicates the "payment" validation phase.
 */
define('UC_TEJARAT_PHASE_PAYMENT', 1);

/**
 * Indicates the "enquiry" validation phase.
 */
define('UC_TEJARAT_PHASE_ENQUIRY', 2);

/**
 * Menu callback for cart/tejarat/complete path, handling gateway response.
 */
function uc_tejarat_complete() {
  // Load UC order object.
  $order = uc_order_load((int) $_SESSION['cart_order']);

  // Check order and bank response availability.
  if ($order && ($response = _uc_tejarat_response())) {
    // Validate transaction result code.
    if (_uc_tejarat_validate(UC_TEJARAT_PHASE_PAYMENT, $response['result_code'])) {
      // Enquiry the transaction remotely.
      $enquiry = _uc_tejarat_enquiry($response['rid']);
      // Validate the enquiry result code.
      // That would be the paid amount in successfull enquiries.
      if (_uc_tejarat_validate(UC_TEJARAT_PHASE_ENQUIRY, $enquiry)) {
        // The transaction has been validated remotely, there
        // were no problems there. Now we need to verify the
        // transaction locally.
        if (_uc_tejarat_verify($order, $response, $enquiry)) {
          // Well, no problems locally. Complete the order:
          _uc_tejarat_finalize($order, $response, $enquiry);
        }
        else {
          // Failure to verify the payment locally. Log the verificaion error.
          watchdog('uc_tejarat', 'Payment local verification failed for order ID %order.', array(
            '%order' => $order_id,
          ), WATCHDOG_ERROR);
        }
      }
      else {
        // Failure in remote payment enquiry.
        watchdog('uc_tejarat', 'Payment enquiry has been failed for order ID %order via SOAP: @error.', array(
          '%order' => $order_id,
          '@error' => _uc_tejarat_translate($enquiry),
        ), WATCHDOG_ERROR);
      }
    }
    else {
      // Failure in payment phase validation, Let user know about the error.
      drupal_set_message(_uc_tejarat_translate($response['result_code']), 'warning');
    }
  }

  // Handle a failed payment, notify user.
  drupal_set_message(t('There were a problem processing your payment via Tejarat Bank, please try again.'), 'error');

  // Redirect to checkout page.
  drupal_goto('cart/checkout');
}

/**
 * Internal helper which finalizes a successfully paid order.
 *
 * @param $order
 *   UC order object to get finalized.
 * @param $response
 *   Payment response.
 * @param $enquiry
 *   Enquiry response which should contain the paid amount logically.
 */
function _uc_tejarat_finalize($order, $response, $enquiry) {
  global $user;

  // Save the transaction information.
  uc_tejarat_save($order, $response, $enquiry);

  // UC order completion workflow...
  $comment = t('Total amount of "!amount" has been successfully paid via "!method". Payment reference is "!reference".',
    array(
      '!reference' => $response['rid'],
      '!method' => ucwords($order->payment_method),
      '!amount' => uc_currency_format($enquiry, TRUE, TRUE, NULL),
    )
  );

  // Enter payment for the order.
  uc_payment_enter($order->order_id, $order->payment_method, $order->order_total, 0, array('reference' => $response['rid']), $comment);
  // Let her know the reference number.
  drupal_set_message($comment);
  // Empty user cart, Update order status, register customer if required
  // and log her in. Also get the themed output of order completion page.
  $message = uc_cart_complete_sale($order, TRUE);
  // Notify user about order completion.
  drupal_set_message($message);
  // Log comments to order, for both customer & admin.
  // Needs to be called after uc_cart_complete_sale().
  uc_order_comment_save($order->order_id, 0, $comment);
  uc_order_comment_save($order->order_id, 0, $comment, 'order', uc_order_state_default('post_checkout'), TRUE);

  // And redirect to the proper end!
  drupal_goto(variable_get('uc_cart_checkout_complete_page', 'cart'));
}

/**
 * Internal helper to return a successfull request response.
 *
 * @return
 *   Either an array of request response or FALSE on unexpected failure.
 */
function _uc_tejarat_response() {
  if (isset($_POST['paymentId'], $_POST['referenceId'], $_POST['resultCode'])) {
    return array(
      'pid' => (int) $_POST['paymentId'],
      'rid' => check_plain($_POST['referenceId']),
      'result_code' => (int) $_POST['resultCode'],
    );
  }
  return FALSE;
}

/**
 * Internal helper which validates transaction result code.
 *
 * @param $phase
 *   Validation phase:
 *   - UC_TEJARAT_PHASE_PAYMENT: Validates payment results.
 *   - UC_TEJARAT_PHASE_ENQUIRY: Validates enquiry results.
 * @param $code
 *   The operation result code.
 *
 * @return
 *   Boolean value indicating the operation validation result.
 */
function _uc_tejarat_validate($phase, $code) {
  // Payment validation phase:
  if ($phase == UC_TEJARAT_PHASE_PAYMENT) {
    return $code == 100 ? TRUE : FALSE;
  }
  // Enquiry validation phase:
  elseif ($phase == UC_TEJARAT_PHASE_ENQUIRY) {
    return ($code > 0) ? TRUE : FALSE;
  }

  return FALSE;
}

/**
 * Internal helper which validates the transaction on the gateway side via SOAP.
 *
 * @param $rid
 *   Transaction's reference id as per returned by the gateway.
 * @param $merchant
 *   Optional merchant code.
 *
 * @return
 *   Either the transaction amount on success or a negetive
 *   result code on transaction enquiry failure. This also
 *   returns FALSE on local errors.
 *
 * @see _uc_tejarat_soap_call()
 */
function _uc_tejarat_enquiry($rid, $merchant = FALSE) {
  // Load the merchant code from configs, if not set.
  if ($rid && ($merchant || ($merchant = variable_get('uc_tejarat_merchant_code', '')))) {
    // Set the SOAP params.
    $soap_params = array(
      'merchantId' => $merchant,
      'referenceNumber' => $rid,
    );

    // Call the remote SOAP method and return the results.
    if ($soap_results = _uc_tejarat_soap_call('verifyRequest', $soap_params)) {
      return $soap_results;
    }
  }
  // Locally invalid merchant.
  return FALSE;
}

/**
 * Internal helper to call a remote SOAP method in the legacy way.
 *
 * Tejarat Bank's service is only compatible with an old version of nuSOAP
 * library which is included in this module. We should drop this library and
 * add a dependency point on soapclient module.
 *
 * @param $soap_method
 *   Remote SOAP method to call.
 * @param $soap_params
 *   SOAP parameters.
 *
 * @return
 *   Either the remote method call results or FALSE on SOAP failure.
 */
function _uc_tejarat_soap_call($soap_method, $soap_params) {
  // Load the legacy nuSOAP library.
  module_load_include('php', 'uc_tejarat', 'includes/nusoap');

  // Instantiate a legacy SOAPClient object.
  $soap_server = _uc_tejarat_gateway_uri('soap');
  $soap_client = new nusoap_client($soap_server);

  // Check for instantiation errors.
  if ($soap_client->getError()) {
    // Log SOAP instantiation error.
    watchdog('uc_tejarat', 'Could not connect to SOAP server: @error', array(
      '@error' => $soap_client->getError()
    ), WATCHDOG_ERROR);
    return FALSE;
  }

  // Set service specific options before remote method call.
  $soap_client->setUseCurl(TRUE);

  // There were no instantiation error, call the remote method.
  $soap_results = $soap_client->call($soap_method, $soap_params, UC_TEJARAT_GATEWAY_NAMESPACE);

  // Check for SOAP call faults.
  if ($soap_client->fault) {
    // Log SOAP method call error.
    watchdog('uc_tejarat', 'Transaction validation failed, fault: @fault', array(
      '@fault' => $soap_results
    ), WATCHDOG_ERROR);
    return FALSE;
  }

  // Check for SOAP call errors.
  if ($soap_client->getError()) {
    watchdog('uc_tejarat', 'Transaction validation failed, error: @error', array(
      '@error' => $soap_client->getError()
    ), WATCHDOG_ERROR);
    return FALSE;
  }

  // No probs, return results.
  return $soap_client->return;
}

/**
 * Internal helper which verifies a paid transaction locally.
 *
 * Checks if the paid amount is equal to the order subtotal, and
 * the sent paymentId and current order_id have the same values. Also
 * it checks the order state to be in "in_checkout" to verify the transaction.
 *
 * @param $order
 *   UC order object.
 * @param $payment_response
 *   Payment response.
 * @param $enquiry_response
 *   Enquiry response.
 *
 * @return
 *   Boolean value indicating the validation result.
 */
function _uc_tejarat_verify($order, $payment_response, $enquiry_response) {
  if ($enquiry_response == $order->order_total &&
      $payment_response['pid'] == $order->order_id &&
      uc_order_status_data($order->order_status, 'state') == 'in_checkout') {
    return TRUE;
  }
  return FALSE;
}

/**
 * Internal helper that translates an operation result code.
 *
 * Returns an operation code's corresponding localizable and human-readable message.
 *
 * @param $code
 *   The transaction result code.
 *
 * @return
 *   Result code corresponding message.
 */
function _uc_tejarat_translate($code) {

}
