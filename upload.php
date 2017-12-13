<?php
//echo $_GET['offset']."<br>";
//echo $_GET['len']."<br>";
//echo $_FILES["file"]["filename"];
//return;

//echo "abc";
//return;
//print_r($_POST);
//print_r($_FILES);
//return;
//echo $_FILES["file"]["type"];
//echo $_POST["name"] ;

//return;
/*
$fp=fopen("1.rar",'wb'); 
fwrite($fp,$_POST['name']);
fclose($fp);
return;



foreach   ($_POST   as   $key   =>   $value)   { 
echo    "post :".$value."<br>"; 
}
return;
*/

echo $_GET['of']."<br>";
echo $_GET['len']."<br>";

$of = $_GET['of'];
$len = $_GET['len'];
if ($_FILES["file"]["size"] < 200000)
  {
  if ($_FILES["file"]["error"] > 0)
    {
    echo "Error: " . $_FILES["file"]["error"] . "<br />";
    }
  else
    {
    echo "Upload: " . $_FILES["file"]["name"] . "<br />";
    echo "Type: " . $_FILES["file"]["type"] . "<br />";
    echo "Size: " . ($_FILES["file"]["size"] ) . " <br />";
    echo "Stored in: " . $_FILES["file"]["tmp_name"];
    }
  }
else
  {
  echo "Invalid file";
  return;
  }

$orgin = $_FILES["file"]["tmp_name"];
$new 	 = basename($_FILES["file"]["tmp_name"]).".bin";  
//$new 	 = "upload.bin";  
move_uploaded_file($orgin,$new);  

//$content = file_get_contents($new);
system("writefile.exe $new console.img $of $len");
unlink($new);
//unlink($orgin);
return;
$filename = "./console.img";
$handle = fopen($filename, "w+");
fseek($handle,$of, SEEK_SET);
fwrite($handle,$content);
fclose($handle);
echo "ok"

?>