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

}

/**
 * Internal helper to return a successfull request response.
 *
 * @return
 *   Either an array of request response or FALSE on unexpected failure.
 */
function _uc_tejarat_response() {

}

