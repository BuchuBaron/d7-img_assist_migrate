<?php

// $query = "SELECT * FROM " . $prefix7 . "field_data_body WHERE body_value like '%[img_assist%'";
function img_assist_migrate_node($nid) {

  $node = db_select('field_data_body', 'fdb')
    ->fields('fdb', array('body_value', 'body_summary'))
    ->condition('entity_id', $nid)
    ->execute();

  $value = '';
  foreach ($node as $mynode) {
    $body = $mynode->body_value;
    $value .= '<h2>Body Value:</h2>' . $body;
    $value .= '<h2>Body Summary:</h2>' . $mynode->body_summary;

    $end = 0;
    while (($start = strpos($body, '[img_assist', $end)) !== FALSE) {
      $end = strpos($body, ']', $start);
      $img = substr($body, $start+12, $end-$start-12);
      $value .= '<h2>Img_assist Tag</h2><p>' . $img . '</p>';
      $start = $end;
    }

    // OK now we start again and pull out the wiki heading tags
    $tags = array(
      '=====' => '<h4>',
      '====' => '<h3>',
      '===' => '<h2>',
      '==' => '<h1>',
    );
    foreach ($tags as $wikicode => $tag) {
      hbdebug("Got $wikicode for $tag");
      $end = 0;
      while (($start = strpos($body, $wikicode, $end)) !== FALSE) {
        $end = strpos($body, $wikicode, $start+strlen($wikicode)) + 1;
        $body = substr_replace($body, $tag, $start, strlen($wikicode));
        $body = substr_replace($body, preg_replace('/</', '</', $tag), $end, strlen($wikicode));
        $end += strlen($wikicode);
        $content = substr($body, $start, $end-$start);
        hbdebug("start=$start, end=$end, content=$content, body=$body");
//        $value .= '<h2>' . $wikicode . ' converted to:</h2>' . $tag . $content . preg_replace('/</', '</', $tag);
        $start = $end;
      }
    }

  }

  return("$value <br />$body");
}
/*
$count=0;
while ($row = mysql_fetch_assoc($result))
{
    $tmp = $row['body_value'];
    $count++;
    echo "\n###############################################################################\n";
    echo "Entity ID: ".$row['entity_id'];
    //echo $tmp;   
    //echo "\n###############################################################################\n";
   
    $start = strpos($tmp, "[img_assist");
   
    while (($start = strpos($tmp, "[img_assist")) !== FALSE) // Added parentheses here since operator precedence was incorrect.
    {
        $end = strpos($tmp, "]", $start);
        $img = substr($tmp, $start+12, $end-$start-12);
       
        echo "Img: $img \n";
       
        list($nid, $title, $desc, $link_img, $align, $width, $height) = explode("|", $img);
       
        $nid = substr($nid, strpos($nid, "=")+1);
        $title = substr($title, strpos($title, "=")+1);
        $desc = substr($desc, strpos($desc, "=")+1);
        $link_img = substr($link_img, strpos($link_img, "=")+1);
        $align = substr($align, strpos($align, "=")+1);
        $width = substr($width, strpos($width, "=")+1);
        $height = substr($height, strpos($height, "=")+1);
       
        echo "NID: $nid \n";
       
        $query_image = "SELECT * FROM " . $prefix6 . "image WHERE nid=".$nid." and image_size='_original'";
        $result_image = mysql_query($query_image,$d6);
            
            if (!$result_image) {
                    $message  = 'Query error : ' . mysql_error($d6) . "\n";
                    $message .= 'Query : ' . $query_image;
                    die($message);
                }
               
        $row_image = mysql_fetch_assoc($result_image);
        $fid = $row_image['fid'];
       
        echo "FID: $fid \n";
       
        $query_file = "SELECT * FROM " . $prefix6 . "files WHERE fid=".$fid;
        $result_file = mysql_query($query_file,$d6);
       
        $row_file = mysql_fetch_assoc($result_file);
        $img_path = $row_file['filepath'];
       
        if ($img_path[0] != '/') 
            $img_path = '/' . $img_path;
       
        echo "Src: $img_path \n";
       
        $buffer = substr($tmp, 0, $start);
       
        $buffer .= "<img alt=\"$desc\" src=\"$img_path\" width=\"$width\" height=\"$height\" class=\"inline inline-$align\" />"; // Not a critical change, but now specifies width and height in the image's attribute tags. Also preserves the alignment of the image if the user specified an alignment through image assist.
       
        $buffer .= substr($tmp, $end+1);
       
        echo "Buffer: $buffer \n";
       
        $tmp = $buffer;
       
        mysql_free_result($result_image);
        //break; // Test
    } // End : while ($start = strpos($tmp, "[img_assist"))   

    $update_query = "UPDATE " . $prefix7 . "field_data_body SET body_value = '".addslashes($tmp)."' WHERE entity_id = ".$row['entity_id'];
    $res = mysql_query($update_query,$d7);
   
    if (!$res) {
        $message  = 'Query error : ' . mysql_error() . "\n";
        $message .= 'Query : ' . $update_query;
        die($message);
    }
   
    //break; // Test
}// End : while ($row = mysql_fetch_assoc($result))

mysql_free_result($result);

mysql_close($d6);
mysql_close($d7);

echo "\nEnd ($count entities modified)\n\n";
*/
