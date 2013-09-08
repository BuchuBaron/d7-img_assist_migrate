<?php

// $query = "SELECT * FROM " . $prefix7 . "field_data_body WHERE body_value like '%[img_assist%'";
function img_assist_migrate_node($nid) {

/*
  $node = db_select('field_data_body', 'fdb')
    ->fields('fdb', array('body_value', 'body_summary'))
    ->condition('entity_id', $nid)
    ->execute();
*/
  $node = node_load($nid);
  hbdebug("node=", $node);

  $value = '';

  $end = 0;
  foreach (array('value', 'summary') as $key) {
    $body = $node->body[$node->language][0][$key];
    $format = $node->body[$node->language][0]['format']; // if format==3 this is legacy wiki
    // for legacy wiki, image files are more likely to be in {files}, so search there first
    // in LoadImageFile.  Otherwise, use file_load to look in {file_managed}
    
    while (($start = strpos($body, '[img_assist', $end)) !== FALSE) {
      $end = strpos($body, ']', $start);
      $img = substr($body, $start+12, $end-$start-12);
      $value .= '<h2>Img_assist Tag</h2><p>' . $img . '</p>';
      $nvpairs = explode('|', $img);
      $img_assist_parms = array();
      foreach ($nvpairs as $pair) {
        list($name, $nvalue) = explode('=', $pair);
        $img_assist_parms[$name] = $nvalue;
        $value .= "$name = $nvalue<br />";
      }
      $img_result = db_query("SELECT * FROM {image} WHERE nid=:img_nid and image_size=:img_size", array(':img_nid' => $img_assist_parms['nid'], ':img_size' => '_original'));
      if ($row = $img_result->fetchAssoc()) {
        $fid = $row['fid'];
        hbdebug("38: got $fid from row=", $row);
        $value .= "Got fid=$fid for nid=" . $img_assist_parms['nid'] . '<br />';
        list($file1, $file2) = LoadImageFile($fid, $format);
      }
      else {
        hbdebug("44:Could not find image for img_assist=", $img_assist_parms);
        exit;
      }
      if ($file1 && $file2) {
        $title = $img_assist_parms['title'];
        $description = $img_assist_parms['desc'];
        $body .=<<<EOS
        <p>Two possible images found here.  Must match $title : $description to either:</p>
EOS;
        foreach (array('legacy' => $file1, 'migrated' => $file2) as $type=>$file) {
          $path = preg_replace('/files/', 'files/styles/medium/public', $file->filepath);
          $title = $img_assist_parms['title'];
          if ($img_assist_parms['align']=='center') {
            $style = "display:block; margin-left:auto; margin-right:auto";
          }
          elseif ($img_assist_parms['align']=='left') {
            $style = "float: left";
          }
          elseif ($img_assist_parms['align']=='right') {
            $style = "float: right";
          }
          $body .=<<<EOS
          <p><a href="/img_assist_migrate/choose_img/$type/$fid/$nid">
             <img alt="" class="image-medium img" src="$path" style="$style" title="$title" /></a></p>
EOS;
        }
        return($body);
      }
      else { // one or other of the two is populated.  stick it in file
        foreach (array('legacy' => $file1, 'migrated' => $file2) as $type=>$testfile) {
          if ($testfile) {
            $file = $testfile;
            if ($type == 'legacy') {
              $legacy = TRUE;
            }
            else {
              $legacy = FALSE;
            }
            break;
          }
        }
      }
      
      $path = preg_replace('/files/', 'files/styles/medium/public', $file->filepath);
      $title = $img_assist_parms['title'];
      if ($img_assist_parms['align']=='center') {
        $style = "display:block; margin-left:auto; margin-right:auto";
      }
      elseif ($img_assist_parms['align']=='left') {
        $style = "float: left";
      }
      elseif ($img_assist_parms['align']=='right') {
        $style = "float: right";
      }
      $buffer =<<<EOS
  <p><a class="colorbox colorbox-insert-image" href="$path" rel="gallery-all" title="$title" ><img alt="" class="image-medium img" src="$path" style="$style" title="$title" /></a></p>
EOS;
      $body = substr_replace($body, $buffer, $start, $end + 1 - $start);
      hbdebug("64:body after replacing img tag with $buffer is", $body);
      $ImgInfo = image_get_info($path);
      hbdebug("66:ImgInfo=", $ImgInfo);

      // Ok, now we insert the DB table rows:
      // Migrate legacy file entries
      $newfid = $fid;
      try {
        if ($legacy) {
          $fields = array(
            'uid' => 1,
            'filename' => basename($path),
            'uri' => 'public://' . basename($path),
            'filemime' => $file->filemime,
            'filesize' => $file->filesize,
            'status' => $file->status,
            'timestamp' => $file->timestamp,
            'type' => 'image',
          );
          hbdebug("Running db_insert('file_managed') with fields=", $fields);
          $newfid = db_insert('file_managed')
            ->fields($fields)
            ->execute();
        }
        if ($newfid && $newfid != $fid) { // legacy file
          hbdebug("deleting fid=$fid from {files}");
          // delete the old entry^
          db_delete('files')
            ->condition('fid', $fid)
            ->execute();
          // set fid to the new value
          $fid = $newfid;
        }
      }
      catch (PDOException $e) {
        drupal_set_message(t('Error: %message', array('%message' => $e->getMessage())), 'error');
      }
       
      // Now insert the image_field records:
      try {
        $fields = array(
          'entity_type' => 'node',
          'bundle' => 'page',
          'deleted' => 0,
          'entity_id' => $nid,
          'revision_id' => $nid,
          'language' => 'und',
          'delta' => 0,
          'field_image_fid' => $fid, 
          'field_image_alt' => $img_assist_parms['desc'],
          'field_image_title' => $img_assist_parms['title'],
          'field_image_width' => $ImgInfo['width'],
          'field_image_height' => $ImgInfo['height'],
        );
        hbdebug("122:db_insert('field_data_field_image') with fields=", $fields);
        db_insert('field_data_field_image')
          ->fields($fields)
          ->execute();

        // Then insert the revision record in field_revision_field_image
        $fields = array(
          'entity_type' => 'node',
          'bundle' => 'page',
          'deleted' => 0,
          'entity_id' => $nid,
          'revision_id' => $nid,
          'language' => 'und',
          'delta' => 0,
          'field_image_fid' => $fid, 
          'field_image_alt' => $img_assist_parms['desc'],
          'field_image_title' => $img_assist_parms['title'],
          'field_image_width' => $ImgInfo['width'],
          'field_image_height' => $ImgInfo['height'],
        );
        hbdebug("db_insert('field_revision_field_image') with fields=", $fields);
        db_insert('field_revision_field_image')
          ->fields($fields)
          ->execute();

          // count the file usage for this file
          file_usage_add(file_load($fid), 'file', 'node', $nid);
        }
      catch (PDOException $e) {
        drupal_set_message(t('Error: %message', array('%message' => $e->getMessage())), 'error');
      }

      $start = $end;
    }

    // replace wiki link tags: [http://example.com the link text]
    $body = preg_replace('/\[([^ ]*) ([^\]]*)\]/s', '<a href="$1">$2</a>', $body);

    // Do the * and # wiki list items
    // 0 - Make sure we only have one type of line-ending to make regexes predictable
    $body = preg_replace('/\r+/s', '', $body);
    // 1 - find the first and last element of each type of list, and put closing tags in
    $body = preg_replace('/^([^*].*)\s*\n\* /m', "$1\n<ul>\n* ", $body);
    $body = preg_replace('/^([^#].*)\s*\n# /m', "$1\n<ol>\n* ", $body);
    $body = preg_replace('/^(\* .*\n)([^*])/m', "$1</ul>\n$2", $body);
    $body = preg_replace('/^(# .*\n)([^#])/m', "$1</ol>\n$2", $body);
    // 2 - replace all list items with their tags
    $body = preg_replace('/^\* (.*)\s*$/m', "<li>$1</li>", $body);
    $body = preg_replace('/^# (.*)\s*$/m', "<li>$1</li>", $body);
    
    // OK now we start again and pull out the wiki heading tags
    $tags = array(
      '=====' => '<h4>',
      '====' => '<h3>',
      '===' => '<h2>',
      '==' => '<h2>',
    );
    foreach ($tags as $wikicode => $tag) {
      $delta = strlen($tag) - strlen($wikicode);
      $end = 0;
      while (($start = strpos($body, $wikicode, $end)) !== FALSE) {
        $end = strpos($body, $wikicode, $start+strlen($wikicode));
        $body = substr_replace($body, $tag, $start, strlen($wikicode));
        $end += $delta; // end shifts out because tag is longer than the wikicode
        $body = substr_replace($body, preg_replace('/</', '</', $tag), $end, strlen($wikicode));
        $end += strlen($wikicode) + 1;
        $start = $end;
      }
    }

    // 3 - mark paragraphs
    $body = preg_replace('/^\s*([^<].*)\s*$/m', "<p>$1</p>", $body);

    $node->body[$node->language][0][$key] = $body;
    $page .= $body;
  }
  $node->body[$node->language][0]['format'] = 2;
  node_save($node);

  return($body);
}

// Loads the file and marks whether it came from legacy or not
// for legacy wiki, image files are more likely to be in {files}, so search there first
// in LoadImageFile.  Otherwise, use file_load to look in {file_managed}
// Feathers - just build up a list of nodes that work the one way or the other.
function LoadImageFile($fid) {
    $my_fresult = db_query("SELECT * FROM {files} WHERE fid=:img_fid", array(':img_fid' => $fid));
    if ($file1 = $my_fresult->fetchObject()) {
      hbdebug("215:Legacy, got fid=$fid from {files} file1=", $file1);
      $legacy = TRUE;
      $file1->filepath = file_create_url($file1->filepath);
    }
    $file2 = file_load($fid);
    if ($file2) {
      hbdebug("221:Migrated fid=$fid file=", $file2);
      $legacy = FALSE;
      $file2->filepath = file_create_url($file2->uri);
    }
/*  }
  else { // format == 2
    $file = file_load($fid);
    if ($file) {
      hbdebug("215:fid=$fid filepath = " . file_create_url($file->uri));
      $legacy = FALSE;
      $file->filepath = file_create_url($file->uri);
    }
    else {
      $my_fresult = db_query("SELECT * FROM {files} WHERE fid=:img_fid", array(':img_fid' => $fid));
      while ($file = $my_fresult->fetchObject()) {
        hbdebug("222:fid=$fid Got file=", $file);
        $legacy = TRUE;
        $file->filepath = file_create_url($file->filepath);
      }
    }
  }
*/
  return(array($file1, $file2));
}
function img_assist_migrate_cleanup_files($type, $fid, $nid) {
  hbdebug("In function img_assist_migrate_cleanup_files($type, $fid, $nid)");
  if ($type=='migrated') { // choose migrated, value in files is legacy so remove it
    db_query("delete from {files} where fid=:file_id", array(':file_id' => $fid,));
  }
  else {
    db_query("delete from {file_managed} where fid=:file_id", array(':file_id' => $fid,));
  }
  drupal_goto("/img_assist_migrate/node/$nid");
}
