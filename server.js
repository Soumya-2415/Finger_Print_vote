// server.js
const express = require("express");
const fs = require("fs");
const path = require("path");
const bodyParser = require("body-parser");
const app = express();
const PORT = 7000;

// Middleware to parse form-data (urlencoded forms)
app.use(express.urlencoded({ extended: true }));

app.use(bodyParser.json()); // Needed to parse JSON bodies

app.get("/", (req, res) => {
  res.sendFile(path.join(__dirname, "WEBPAGE FOR DATABASE.html"));
});

app.post("/saveapi", (req, res) => {
  console.log(req.body);
  const { serial, name, dob, voterid, gender } = req.body;

  if (!serial || !name || !dob || !voterid || !gender) {
    return res.status(400).send("All fields are required.");
  }

  // Format line: Serial, Name, DOB, VoterID, Gender
  const line = `${serial},${name},${dob},${voterid},${gender}\n`;

  const filePath = path.join(__dirname, "voter.txt");

  fs.appendFile(filePath, line, (err) => {
    if (err) {
      console.error("Error writing to file:", err);
      return res.status(500).send("Failed to save data.");
    }
    res.send("Data saved successfully.");
  });
});

// Start server
app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
});
