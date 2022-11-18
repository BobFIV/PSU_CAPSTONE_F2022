import React from "react";
import "./App.css";
import DashboardComponent from "./Dashboard.js";

function App() {
  /*return (
     <DashboardComponent 
         defaultCSE="http://34.238.135.110:8080"
         defaultOriginator="Cdashboard" 
         defaultRI="id-in" />
  );*/
  return (
     <DashboardComponent 
         defaultCSE="http://localhost:8081"
         defaultOriginator="Cdashboard" 
         defaultRI="id-in" />
  );
}

export default App;
