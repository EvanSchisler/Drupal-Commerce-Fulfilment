<?php
/**
 * @param $order
 * @return string
 *
 * Builds the package page.
 * Pulls in Order info and displays the name of the products in chosen Order.
 *
 */
function commerce_fulfilment_package_content($order){
  $order_wrapper = entity_metadata_wrapper( 'commerce_order', $order);
  $order_id = $order_wrapper->order_id->value();

  $packages = entity_load('package', FALSE, array('order_id'=>$order_id));

  $output = t('<table><tr><td style = vertical-align:top>');
  $output .= t('<h2>' . t('Products in Order ' .$order_id . ':') . '</h2>');
  $output .= views_embed_view('commerce_fulfilment_line_items', 'default', $order_id);
  $output .= t('</td><td style = vertical-align:top><h2>Created Packages for This Order:</h2>');

  //outputs each package
  foreach ($packages as $package) {
    $package_wrapper = entity_metadata_wrapper('package', $package);
    $output .= t('<h3>Package ' . $package_wrapper->package_id->value() . ':</h3>');
    $output .= t('<p>&emsp;<strong>Type: </strong>' . $package_wrapper->box_type->value() . '</p>');
    $output .= t('<h4>Products in Package:</h4>');
    $line_items = array();

    //creates an array of line_items that are in the specific package
    foreach($package_wrapper->commerce_fulfilment_line_items->value() as $line_item){
      $line_items[] = $line_item;
    }
    //outputs each line_item that are in the specific package
    foreach ($line_items as $line_item) {
      $output .= t('<p><strong>Product ID: </strong>' . $line_item->line_item_id
        . '</p><p>&emsp;<strong>Product Code: </strong>' . $line_item->line_item_label . '</p>');
      $output .= t('<p>&emsp;<strong>Quantity: </strong>' . $line_item->quantity . '</p>');
    }

    //outputs form to interact with the package
    $get_form = drupal_get_form('commerce_fulfilment_manipulate_product_form_' . $package_wrapper->package_id->value(),
      array('0' => $package_wrapper->package_id->value(), '1' => $order_id));
    $output .= drupal_render($get_form);
  }
  //outputs forms to create a package and to print the packing slip for the selected package
  $output.= t('</td></tr><tr>');
  $get_form = drupal_get_form('commerce_fulfilment_create_package_form', $order_id);
  $output .= t('<td>' . drupal_render($get_form) . '</td>');
  $get_form = drupal_get_form('commerce_fulfilment_packing_slip_form', $order_id);
  $output .= t('<td>' . drupal_render($get_form) . '</td>');
  $output .= t('</tr></table>');
  return $output;
}

/**
 * @param $form
 * @param $form_state
 * @param $order_id
 * @return array
 *
 * implements hook_form
 * adds functionality to create a package
 *
 */
function commerce_fulfilment_create_package_form($form, &$form_state,$order_id){

  $package_type = variable_get('package_type', array(0 => 'No Package Type set.'));
  $form = array();
  $form['commerce_fulfilment_order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id,
  );
  $form['commerce_fulfilment_title'] = array(
    '#markup'=>'<h2>Create a New Package:</h2>'
  );
  $form['commerce_fulfilment_package_type'] = array(
    '#type' => 'select',
    '#title' => 'Select package type: ',
    '#options' => $package_type,
    '#default_value' => 'default',
  );
  $form['commerce_fulfilment_create'] = array(
    '#type' => 'submit',
    '#value' => t('Create a Package')
  );
  return $form;
}

/**
 * @param $form
 * @param $form_state
 *
 * implements hook_form_submit
 */
function commerce_fulfilment_create_package_form_submit($form, &$form_state){
  $package_type = variable_get('package_type');
  //create an order entity wrapper
  $order_id = $form_state['values']['commerce_fulfilment_order_id'];
  $order = commerce_order_load($order_id);
  $order_wrapper= entity_metadata_wrapper('commerce_order', $order);
  //create a package entity wrapper
  $profile_id = $order_wrapper->commerce_customer_billing->profile_id->value();
  $profile = commerce_customer_profile_load($profile_id);
  $profile_wrapper = entity_metadata_wrapper('commerce_customer_profile', $profile);

  //create the package entity
  $entity = entity_create('package', array(
    'order_id' => $order_wrapper->order_id->value(),
    'box_type' => $package_type[$form_state['values']['commerce_fulfilment_package_type']],
    'shipping_info'=> array(
      'firstName' => $profile_wrapper->commerce_customer_address->first_name->value(),
      'lastName' => $profile_wrapper->commerce_customer_address->last_name->value(),
      'company' => $profile_wrapper->commerce_customer_address->organisation_name->value(),
      'address' => $profile_wrapper->commerce_customer_address->thoroughfare->value(),
      'city' => $profile_wrapper->commerce_customer_address->locality->value(),
      'state' => $profile_wrapper->commerce_customer_address->administrative_area->value(),
      'zip' => $profile_wrapper->commerce_customer_address->postal_code->value(),
      'country' => $profile_wrapper->commerce_customer_address->country->value(),
    ),
  ));

  //insert entity into database
  $entity->save('package', $entity);
}
/**
 * @param $form_id
 * @param $args
 * @return array
 *
 * implements hook_forms
 * Allows for multiple forms to be applied to each page, each with their separate id and submit button.
 */
function commerce_fulfilment_forms($form_id, $args){
  $forms = array();
  if (strpos($form_id, 'commerce_fulfilment_manipulate_product_form_') !== FALSE) {
    $forms[$form_id] = array(
      'callback' => 'commerce_fulfilment_manipulate_product_form_builder',
      'callback arguments' => $args,
    );
  }
  if (strpos($form_id, 'commerce_fulfilment_manipulate_package_form_') !== FALSE) {
    $forms[$form_id] = array(
      'callback' => 'commerce_fulfilment_manipulate_package_form_builder',
      'callback arguments' => $args,
    );
  }
  return $forms;
}

/**
 * @param $form
 * @param $form_state
 * @return array
 *
 * implements hook hook_form
 * Adds the select box and add button to the packages interface.
 */
function commerce_fulfilment_manipulate_product_form_builder($form, &$form_state, $args){
  $package_id = $args[0];
  $order_id = $args[1];
  $form = array();
  $options = array(0 => t('Add Product'),1 => t('Remove Product'), 2 => t('Delete Package'));
  $form['commerce_fulfilment_package_id'] = array(
    '#type' => 'hidden',
    '#value' => $package_id,
  );
  $form['commerce_fulfilment_order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id,
  );
  $form['commerce_fulfilment_product'] = array(
    '#type' => 'select',
    '#title' => t('Select Product to Add: '),
    '#options' => commerce_fulfilment_get_product_array($order_id),
  );
  $form['commerce_fulfilment_radios'] = array(
    '#type' => 'radios',
    '#title'=>t('Options'),
    '#default value' => $options[0],
    '#options' => $options
  );
  $form['commerce_fulfilment_add'] = array(
    '#type' => 'submit',
    '#value' => t('Submit')
  );
  $form['#submit'][] = 'commerce_fulfilment_manipulate_product_form_submit';
  return $form;
}
/**
 * @param $order_id
 * @param $product
 *
 * @return array
 *
 * loads an array of line item id's.
 * If there is no index chosen, Returns the array.
 * If there is an index chosen. Returns the data in the array at that index.
 */
function commerce_fulfilment_get_product_array($order_id, $product = FALSE){
  $order = commerce_order_load($order_id);
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  $line_ids = array();

  foreach($order_wrapper->commerce_line_items->value() as $index => $line_item){
    $line_ids[] = $line_item->line_item_id;
  }

  if($product !== FALSE && is_int($product) && $product <= count($line_ids)) {
    return $line_ids[$product];
  }
  return $line_ids;
}
/**
 * @param $form
 * @param $form_state
 * implement hook hook_form_submit
 * Implements the functionality behind commerce_fulfilment_manipulate_product_form
 *
 */
function commerce_fulfilment_manipulate_product_form_submit($form, &$form_state){
  $order_id = $form_state['values']['commerce_fulfilment_order_id'];
  $package_id[0] = $form_state['values']['commerce_fulfilment_package_id'];

  $package = entity_load('package', $package_id, array('order_id' => $order_id));
  $package_wrapper = entity_metadata_wrapper('package', $package[$package_id[0]]);

  $products = array();

  foreach($package_wrapper->commerce_fulfilment_line_items->value() as $line_item){
    $products[] = $line_item;
  }

  $argument = commerce_fulfilment_get_product_array($order_id, (int)$form_state['values']['commerce_fulfilment_product']);

  //add a product to a package
  if($form_state['values']['commerce_fulfilment_radios'] == 0) {
    $count= 0;
    foreach($package_wrapper->commerce_fulfilment_line_items->value() as $index => $line_item){
      $line_item = entity_metadata_wrapper('commerce_line_item', $line_item);
      if($line_item->line_item_id->value() == $argument){
        $count++;
      }
    }
    if($count == 0) {
      $package_wrapper->commerce_fulfilment_line_items[] = $argument;
      $package_wrapper->save();
    }
    else{
      drupal_set_message('Product ' . $argument . ' is already in Package ' . $package_id[0] . '.');
    }
  }
  //remove a product from a package
  elseif($form_state['values']['commerce_fulfilment_radios'] == 1) {
    $count = 0;
    $success_count = 0;
    foreach ($products as $line_item) {
      $line_item = entity_metadata_wrapper('commerce_line_item', $line_item);
      if ($line_item->line_item_id->value() == $argument) {
        unset($package_wrapper->commerce_fulfilment_line_items[$count]);
        $package_wrapper->save();
        drupal_set_message('Product ' . $argument . ' was removed from Package ' . $package_id[0] . '.');
        $success_count++;
      }
      $count++;
    }
    if($success_count == 0){
      drupal_set_message('Product ' . $argument . ' was not found in Package ' . $package_id[0] . '.');
    }
  }
  //delete package
  elseif($form_state['values']['commerce_fulfilment_radios']==2){
    $package[$package_id[0]]->delete();
    drupal_set_message('Package ' . $package_id[0] . 'was deleted');
  }
}

/**
 * @param $form
 * @param $form_state
 * @param $order_id
 * @return array
 *
 * Implements hook_form
 * Shows form for printing packing slips
 */
function commerce_fulfilment_packing_slip_form($form, $form_state, $order_id){
  $package_arr['und'] = entity_load('package', FALSE, array('order_id' => $order_id));
  $form = array();
  $form['commerce_fulfilment_order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id,
  );
  $form['commerce_fulfilment_title'] = array(
    '#markup'=>'<h2>Print Packing Slip:</h2>'
  );
  $form['commerce_fulfilment_package'] = array(
    '#type' => 'select',
    '#title' => t('Select Package:'),
    '#options' => commerce_fulfilment_get_package_array($package_arr),
  );
  $form['commerce_fulfilment_print'] = array(
    '#type' => 'submit',
    '#value' => t('Print Packing Slip')
  );
  return $form;
}

/**
 * @param $form
 * @param $form_state
 * redirects the user to the packing slip
 */
function commerce_fulfilment_packing_slip_form_submit($form, &$form_state){
  $package_arr['und'] = entity_load('package', FALSE, array('order_id' => $form_state['values']['commerce_fulfilment_order_id']));
  $argument = commerce_fulfilment_get_package_array($package_arr, (int)$form_state['values']['commerce_fulfilment_package']);

  $form_state['redirect'] = 'admin/commerce/orders/' . $form_state['values']['commerce_fulfilment_order_id'] .'/commerce_fulfilment/' . $argument . '/packing_slip';
}

/**
 * @param $package_id
 *
 * The page to show and print the packing slip
 *
 */
function commerce_fulfilment_packing_slip_content($package_id){
  $package_selected = entity_load('package', FALSE, array('package_id' => $package_id));
  $package_wrapper = entity_metadata_wrapper('package', $package_selected[$package_id]);
  $order_id = $package_selected[$package_id]->order_id;
  $packages = entity_load('package', FALSE, array('order_id' => $order_id));
  $products = $package_wrapper->commerce_fulfilment_line_items->value();
  $count = 0;
  foreach($packages as $package){
    $count++;
  }
  $image = variable_get('commerce_fulfilment_logo', NULL);
  if($image !== NULL) {
    $image_obj = file_load($image);
    if (isset($image_obj->uri)) {
      $image_obj_url = file_create_url($image_obj->uri);
    }
  }
  else {
    $image_obj_url = NULL;
  }
  $output = t('<table width = "700" border = "1"><tr><td width = "20%"><img src = "'. $image_obj_url .'"></td>');
  $output .= t('<td width = "80%"><p></p><strong>Company Info.: <br></strong>&emsp;'
    . variable_get('commerce_fulfilment_company', NULL) . '</p>&emsp;'
    . variable_get('commerce_fulfilment_address', NULL) . '<br>&emsp;'
    . variable_get('commerce_fulfilment_phone', NULL)
    . '</td></tr>');

  $output .= t('<tr><td style = vertical-align:top align = "top" height = "500" colspan ="2">
        <table cellpadding = "15" width = "700" >');

  $output .= t('<th align = "left"> Product Code:</th><th align = "left"> Quantity</th>
    <th align = "left"> Unit Price:</th><th align = "left"> Total Price:</th>');

  foreach($products as $product){

    $product_wrapper = entity_metadata_wrapper('commerce_line_item', $product);
    $prices[0] = $product_wrapper->commerce_unit_price->amount->value();
    $prices[1] = $product_wrapper->commerce_total->amount->value();

    $length = strlen($prices[0]);
    $prices[0] = substr_replace($prices[0], '.', $length - 2, 0);
    $length = strlen($prices[1]);
    $prices[1] = substr_replace($prices[1], '.', $length - 2, 0);

    $output .= t('<tr><td>' . $product->line_item_label . '</td><td>' . $product->quantity .'</td><td>' .
      $prices[0] . ' ' . $product_wrapper->commerce_unit_price->currency_code->value() . '</td><td>' . $prices[1] . ' '
      . $product_wrapper->commerce_total->currency_code->value() . '</td></tr>');
  }
  $output .= t('</table></td></tr>');
  $output .= t('<tr><td colspan = "2"><strong>Package ID: </strong>' . $package_wrapper->package_id->value()
    . '<br><strong>Package:</strong> 1 of ' . $count . '</td>');
  $output.= t('</tr></table>');
  print $output;
}
/**
 * @return string
 *
 * Builds the shipments page
 *
 */
function commerce_fulfilment_shipment_content($order) {

  $order_id = $order->order_id;
  //dpm($order);
  $packages = entity_load('package', FALSE, array('order_id'=>$order_id));
  $packages_id = array();
  $shipments = entity_load('shipment', FALSE, array('order_num'=>$order_id));
  //dpm($packages);
  $output = t('<table border = "0"><tr><td style = vertical-align:top><h2>Created Packages for This Order:</h2>');
  foreach ($packages as $package) {
    $package_wrapper = entity_metadata_wrapper('package', $package);
    $package_id = $package_wrapper->package_id->value();
    $packages_id[] = $package_wrapper->package_id->value();
    $output .= t('<h3>Package ' . $package_id . ':</h3>');
    $output .= t('<h4>Products in Package:</h4>');
    $line_items = array();

    foreach($package_wrapper->commerce_fulfilment_line_items->value() as $line_item){
      $line_items[] = $line_item;
    }

    foreach ($line_items as $line_item) {
      $line_item = entity_metadata_wrapper('commerce_line_item', $line_item);
      $output .= t('<p><strong>Product ID: </strong>' .$line_item->line_item_id->value()
        . '</p><p><strong>Product Code: </strong>' . $line_item->line_item_label->value() . '</p>');
    }
  }
  $output .= t('</td><td style = vertical-align:top><h2>Created Shipments for This Order:</h2>');
  foreach ($shipments as $shipment) {
    $shipment_id = $shipment->shipment_id;
    $output .= t('<h3>Shipment ' . $shipment_id . ':</h3>');
    $output .= t('<p>&emsp;<strong>Tracking Number: </strong>' . $shipment->tracking_number . '</p>');
    $output .= t('<p>&emsp;<strong>Date Created: </strong>'
      . date("Y M d", strtotime($shipment->dates['created'])) . '</p>');
    $output .= t('<p><strong>&emsp;Address: </strong>' . $shipment->shipping_info['address'] .
      ', ' . $shipment->shipping_info['city'] . ' ' . $shipment->shipping_info['state'] .
      ', ' . $shipment->shipping_info['country'] . ' ' . $shipment->shipping_info['zip'] . '</p>');

    $output .= t('<h4>&emsp;Packages in Shipment:</h4>');
    $package_ids = $shipment->package_num['und'];
    if($package_ids != NULL) {
      foreach ($package_ids as $package_id) {
        if ($package_id != NULL) {
          $package = entity_load('package', FALSE, array('package_id' => $package_id));
          $package_wrapper = entity_metadata_wrapper('package', $package[$package_id]);
          $output .= t('<p>&emsp;&emsp;<strong>Package ID: </strong>' . $package_wrapper->package_id->value()
            . '</p>');
        }
      }
    }
    $args = array($shipment_id);
    $get_form = (drupal_get_form('commerce_fulfilment_manipulate_package_form_' . $shipment_id, $args));
    $output .= drupal_render($get_form);
  }
  $output .= t('</td></tr><tr><td>');
  $get_form = drupal_get_form('commerce_fulfilment_create_shipment_form', $order_id);
  $output .= drupal_render($get_form) . '</td>';
  $get_form = drupal_get_form('commerce_fulfilment_shipping_label_print_form', $order_id);
  $output .= t('<td>' . drupal_render($get_form) . '</td></tr></table>');

  return $output;
}
/**
 * @param $form
 * @param $form_state
 * @return array
 *
 * implements hook hook_form
 * Adds the select box and add button to the packages interface.
 */
function commerce_fulfilment_manipulate_package_form_builder($form, &$form_state, $args){
  $options = array(0 => t('Add Package'),1 => t('Remove Package'), 2 => t('Delete Shipment'));
  $shipment_id = $args[0];
  $shipment = entity_load('shipment', FALSE, array('shipment_id'=>$shipment_id));
//    dpm($shipment);
  $order_id = $shipment[$shipment_id]->order_num;
  $package_arr['und'] = entity_load('package', FALSE, array('order_id' => $order_id));
  //dpm($package_arr);
  $form['commerce_fulfilment_order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id,
  );
  //dpm($package_arr);
  $form['commerce_fulfilment_shipment_id'] = array(
    '#type' => 'hidden',
    '#value' => $shipment_id,
  );
  $form['commerce_fulfilment_package'] = array(
    '#type' => 'select',
    '#title' => t('Select Package: '),
    '#options' => commerce_fulfilment_get_package_array($package_arr),
  );
  $form['commerce_fulfilment_radios'] = array(
    '#type' => 'radios',
    '#title'=>t('Options'),
    '#default value' => $options[0],
    '#options' => $options
  );
  $form['commerce_fulfilment_add'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  $form['#submit'][] = 'commerce_fulfilment_manipulate_package_form_submit';
  return $form;
}

/**
 * @param $form
 * @param $form_state
 * @param $order_id
 * @return array
 * implements hook form
 * implements functionality to select which shipment to print shipping labels for.
 *
 */
function commerce_fulfilment_shipping_label_print_form($form, &$form_state, $order_id){
  $shipment_arr['und'] = entity_load('shipment', FALSE, array('order_num' => $order_id));
  //dpm($shipment_arr);
  $form = array();
  $form['commerce_fulfilment_order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id,
  );
  $form['commerce_fulfilment_title'] = array(
    '#markup'=>'<h2>Print Shipping Label:</h2>'
  );
  $form['commerce_fulfilment_shipment'] = array(
    '#type' => 'select',
    '#title' => t('Select Shipment:'),
    '#options' => commerce_fulfilment_get_shipment_array($shipment_arr),
  );
  $form['commerce_fulfilment_print'] = array(
    '#type' => 'submit',
    '#value' => t('Print Shipping Label')
  );
  return $form;
}

/**
 * @param $form
 * @param $form_state
 * redirects the user to the shipping label page
 */
function commerce_fulfilment_shipping_label_print_form_submit($form, &$form_state){
  $order_id = $form_state['values']['commerce_fulfilment_order_id'];
  $shipment_arr['und'] = entity_load('shipment', FALSE, array('order_num' => $order_id));
  $argument = commerce_fulfilment_get_shipment_array($shipment_arr, (int)$form_state['values']['commerce_fulfilment_shipment']);
  $form_state['redirect'] = 'admin/commerce/orders/' . $order_id .'/commerce_fulfilment/' . $argument . '/shipping_label';
}

/**
 * @param $shipment_id
 * @return string
 *
 * Prints the shipping label on its own page. Takes in the shipment_id and loads the shipment.
 * Prints Company name, address, phone # and logo from the administrator page.
 *
 */
function commerce_fulfilment_shipping_label_content($shipment_id){
  $shipment_arr = entity_load('shipment', FALSE, array('shipment_id' => $shipment_id));
  $shipment = $shipment_arr[$shipment_id];
  $image = variable_get('commerce_fulfilment_logo', NULL);
  if($image !== NULL) {
    $image_obj = file_load($image);
    if (isset($image_obj->uri)) {
      $image_obj_url = file_create_url($image_obj->uri);
    }
  }
  else {
    $image_obj_url = NULL;
  }
  $output = t('<table border = "1px solid black" height = "350" width = "700"><tr><td height = "50%">
    <strong> From: </Strong>' . variable_get('commerce_fulfilment_company', NULL) . '<br>&emsp;&emsp;&emsp;'
    . variable_get('commerce_fulfilment_address', NULL)
    . '<br>&emsp;&emsp;&emsp;' . variable_get('commerce_fulfilment_phone', NULL) . '</td>');
  $output .= t('<td align = "center"><img src = "' . $image_obj_url .  '")></td></tr>');
  $output .= t('<tr><td height = "50%"><strong>To: </strong>' . $shipment->shipping_info['firstName'] . ' '
    . $shipment->shipping_info['lastName'] . '<br>'
    . $shipment->shipping_info['address'] . '<br>' . $shipment->shipping_info['city'] . ', '
    . $shipment->shipping_info['state'] . ', ' . $shipment->shipping_info['country'] . '<br>'
    . $shipment->shipping_info['zip'] . '</td>');
  $output .= t('<td width = "50%"><strong>Order ID: </strong>' . $shipment->order_num
    . '<br><strong>Tracking Number: </strong>' . $shipment->tracking_number . '<br><strong>Shipping Method: </strong>'
    . $shipment->method['method'] . '<br>' . '</tr></table>');
  print $output;
}
/**
 * @param $shipments
 * @param bool $index
 * @return array
 * returns an array of shipment ids if there is no index.
 * If there is an index returns the selected shipment ID
 *
 */
function commerce_fulfilment_get_shipment_array($shipments, $index = FALSE) {
  $shipments = $shipments['und'];
  $shipment_id = array();
  $count = 0;

  //Creates an array of package id's for the select box.

  foreach ($shipments as $shipment) {
    $shipment_id[$count] = $shipment->shipment_id;
    $count++;
  }
  if($index !== FALSE && is_int($index) && $index <= count($shipment_id)) {
    return $shipment_id[$index];
  }
  return $shipment_id;
}
/**
 * @param $packages
 * @param bool $index
 * @return array
 * Gets an array of package_ids to display in the commerce_fulfilment_manipulate_package_form_builder
 */
function commerce_fulfilment_get_package_array($packages, $index = FALSE){
  $packages = $packages['und'];
  $package_id = array();
  $count = 0;

  //Creates an array of package id's for the select box.

  foreach ($packages as $package) {
    $package_id[$count] = $package->package_id;
    $count++;
  }
  if($index !== FALSE && is_int($index) && $index <= count($package_id)) {
    return $package_id[$index];
  }
  return $package_id;
}

/**
 * @param $form
 * @param $form_state
 *
 * Implements hook_form_submit,
 * Places a package into a shipment
 */
function commerce_fulfilment_manipulate_package_form_submit(&$form, &$form_state){
  //loads shipment
  $shipment_id = $form_state['values']['commerce_fulfilment_shipment_id'];
  $shipment = entity_load('shipment', FALSE, array('shipment_id'=>$shipment_id));

  //loads an array of package entities in the shipment
  $order_id = $form_state['values']['commerce_fulfilment_order_id'];
  $package_arr['und'] = entity_load('package', FALSE, array('order_id' => $order_id));
  $package_id = (int)$form_state['values']['commerce_fulfilment_package'];

  //gets the argument to add/remove from the shipment
  $arg = commerce_fulfilment_get_package_array($package_arr , $package_id);

  //checks if there is a package with the argument ID
  $package = $package_arr['und'][$arg];
  $package_ids = $shipment[$shipment_id]->package_num['und'];
  $package_id = $package->package_id;

  if($form_state['values']['commerce_fulfilment_radios'] == 0 ) {
    $count = 0;
    if(isset($package_ids)) {
      foreach ($package_ids as $pack_id) {
        if ($pack_id == $arg) {
          $count++;
        }
      }
    }
    if($count == 0) {
      $shipment[$shipment_id]->package_num['und'][] = $package_id;
      $shipment[$shipment_id]->save('shipment', $shipment);
    }
    else{
      drupal_set_message('Package ' . $arg . ' is already in Shipment ' . $shipment_id . '.');
    }
  }
  elseif($form_state['values']['commerce_fulfilment_radios'] == 1) {
    $success_count = 0;
    for($counting = count($package_ids)-1; $counting>=0; $counting--) {

      if ($shipment[$shipment_id]->package_num['und'][$counting] == $package_id) {
        $shipment[$shipment_id]->package_num['und'][$counting] = NULL;
        $shipment[$shipment_id]->save('shipment', $shipment);
        $success_count++;
      }
    }
    if($success_count == 0){
      drupal_set_message('Package ' . $arg . ' was not found in Shipment ' . $shipment_id . '.');
    }
  }
  elseif($form_state['values']['commerce_fulfilment_radios'] == 2){
    $shipment[$shipment_id]->delete();
    drupal_set_message('Shipment ' . $shipment_id . ' deleted.');
  }
}
/**
 * @param $form
 * @param $form_state
 * @param $order_id
 * @return array
 *
 * Implements hook_form
 * Implements form to create a new shipment object
 * Allows user to set the tracking number if desired
 *
 */

function commerce_fulfilment_create_shipment_form($form, &$form_state,$order_id){

  $form = array();
  $form['commerce_fulfilment_order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order_id,
  );
  $form['commerce_fulfilment_title'] = array(
    '#markup'=>'<h2>Create a New Shipment:</h2>'
  );
  $form['commerce_fulfilment_tracking_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Enter tracking number:'),
    '#size' => 20,
    '#length' => 255,
    '#required' => false,
  );
  $form['commerce_fulfilment_create'] = array(
    '#type' => 'submit',
    '#value' => t('Create Shipment')
  );
  return $form;
}

function commerce_fulfilment_create_shipment_form_submit(&$form, &$form_state){

  $order_id = $form_state['values']['commerce_fulfilment_order_id'];
  $order = commerce_order_load($order_id);
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  $profile = commerce_customer_profile_load($order_wrapper->commerce_customer_shipping->profile_id->value());
  $profile_wrapper = entity_metadata_wrapper('commerce_customer_profile', $profile);

  $entity_type = 'shipment';
  $entity = entity_create($entity_type, array(
    'order_num' => $order->order_id,
    'status' => 'pending',
    'packages' => '',
    'dates' => array(
      'created' => date('ymd'),
    ),
    'shipping_info' => array(
      'firstName' => $profile_wrapper->commerce_customer_address->first_name->value(),
      'lastName' => $profile_wrapper->commerce_customer_address->last_name->value(),
      'company' => $profile_wrapper->commerce_customer_address->organisation_name->value(),
      'address' => $profile_wrapper->commerce_customer_address->thoroughfare->value(),
      'city' => $profile_wrapper->commerce_customer_address->locality->value(),
      'state' => $profile_wrapper->commerce_customer_address->administrative_area->value(),
      'zip' => $profile_wrapper->commerce_customer_address->postal_code->value(),
      'country' => $profile_wrapper->commerce_customer_address->country->value(),
    ),
    'tracking_number' => $form_state['values']['commerce_fulfilment_tracking_number'],
  ));
  $entity->save('shipment', $entity);
}