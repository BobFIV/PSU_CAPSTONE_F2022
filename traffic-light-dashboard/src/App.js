import React from "react";
import "./App.css";
import { Dashboard, DashboardComponent } from "./Dashboard.js";

var dash = new Dashboard();

function App() {
  return (
     <DashboardComponent 
         dashboard={dash} 
         defaultCSE="http://34.238.135.110:8080"
         defaultOriginator="Cdashboard" 
         defaultRI="id-in" />
  );
}

export default App;
