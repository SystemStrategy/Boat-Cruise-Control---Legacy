const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <body>
    <h3 style="text-align:left">Parameters<h3>
    <table frame="box" style="background-color:#D3D3D3;">
      <tr style="background-color:#A6A6A6;"><th colspan="2">Status</th></tr>
      <TR><TD>GPS Fix:</TD><TD>@@gpsfix@@ </TD></TD>
      <TR><TD>GPS MS:</TD><TD>@@gpshz@@ </TD></TD>
      <TR><TD>GPS MS Min:</TD><TD>@@gpsmin@@ </TD></TD>
      <TR><TD>GPS MS Max:</TD><TD>@@gpsmax@@ </TD></TD>
      <TR><TD>GPS Speed:</TD><TD>@@gpsspeed@@ </TD></TD>
      <TR><TD>Motor Position</TD><TD>@@motorposition@@ </TD></TD>
      <TR><TD>Proportional</TD><TD>@@prop@@ </TD></TD>
      <TR><TD>Integral</TD><TD>@@integ@@ </TD></TD>
      <TR><TD>Derivative</TD><TD>@@deriv@@ </TD></TD>
    </TABLE>
    <br>
    <h3 style="text-align:left">Boat Tuning Parameters<h3>
    <br>
    <FORM method="post" action="/motor_params">  
      <table frame="box" style="background-color:#D3D3D3;">
        <tr style="background-color:#A6A6A6;"><th colspan="2">Stepper Motor Params</th></tr>
        <TR><TD>Reverse Direction: </TD><TD><input type="checkbox" name="MRev" value="1" @@MRev@@></TD></TD>
        <TR><TD>Motor steps per rotation: </TD><TD><input type="text" name="MSteps" value=@@MSteps@@></TD></TR>
        <TR><TD>Motor Speed: </TD><TD><input type="text" name="MSpeed" value=@@MSpeed@@></TD></TR>
        <TR><TD>Motor Max Rotation Steps: </TD><TD><input type="text" name="Mmax" value=@@Mmax@@></TD></TR>
        <TR><TD colspan="2"> <input type="submit" value="Submit"></TD></TR>
      </TABLE>
    </form>  
    <br>
    <FORM method="post" action="/tuning_params">  
      <table frame="box" style="background-color:#D3D3D3;">
        <tr style="background-color:#A6A6A6;"><th colspan="2">Tuning Parameters</th></tr>
        <TR><TD>KP: </TD><TD><input type="text" name="kp" value=@@kp@@></TD></TR>
        <TR><TD>KI: </TD><TD><input type="text" name="ki" value=@@ki@@></TD></TR>
        <TR><TD>KD: </TD><TD><input type="text" name="kd" value=@@kd@@></TD></TR>
        <TR><TD>Deadband: </TD><TD><input type="text" name="deadband" value=@@deadband@@></TD></TR>
        <TR><TD>Ramp Rate: </TD><TD><input type="text" name="ramp" value=@@ramp@@></TD></TR>
        <TR><TD colspan="2"> <input type="submit" value="Submit"></TD></TR>
      </TABLE>
    </form>
    <br>
    <FORM method="post" action="/defaults">  
      <table frame="box" style="background-color:#D3D3D3;">
        <tr style="background-color:#A6A6A6;"><th colspan="2">Default Speeds</th></tr>
        <TR><TD>Surf: </TD><TD><input type="text" name="Default1" value=@@Default1@@></TD></TR>
        <TR><TD>Wakeboard: </TD><TD><input type="text" name="Default2" value=@@Default2@@></TD></TR>
        <TR><TD>Ski: </TD><TD><input type="text" name="Default3" value=@@Default3@@></TD></TR>
        <TR><TD colspan="2"> <input type="submit" value="Submit"></TD></TR>
      </TABLE>
    </form>
    <br>
    <FORM method="post" action="/save">  
      <table frame="box" style="background-color:#D3D3D3;">
        <tr style="background-color:#A6A6A6;"><th colspan="2">Save Settings</th></tr>
        <TR><TD><input type="submit" value="Save"></TD></TR>
      </TABLE>
    </form>
    <br>
    <FORM method="post" action="/download_data">  
      <table frame="box" style="background-color:#D3D3D3;">
        <tr style="background-color:#A6A6A6;"><th colspan="2">Retrive 2 minute Data File</th></tr>
        <TR><TD><input type="submit" value="Data.txt"></TD></TR>
      </TABLE>
    </form>
    <br>
    <form method='POST' action="/update" enctype='multipart/form-data' id='upload_form'>    
      <table frame="box" style="background-color:#D3D3D3;">
        <tr style="background-color:#A6A6A6;"><th colspan="2">Update Firmware</th></tr>
        <TR><TD><input type='file' name='update'></TD><TD> <input type='submit' value='Update'></TD></TR>
      </TABLE>
    </form>
  </body>
</html>
)=====";
