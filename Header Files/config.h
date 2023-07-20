<!DOCTYPE html>
<html>
  <head>
    <title>Set Warning Time</title>
    <style>
      body {
        width: 300px;
        padding: 10px;
      }
    </style>
    <script src="popup.js"></script>
  </head>
  <body>
    <h1>Set Warning Time</h1>
    <form id="timeForm">
      <label for="time">Time in seconds:</label><br>
      <input type="number" id="time" name="time" min="1"><br>
      <input type="submit" value="Set Time">
    </form>
  </body>
</html>
