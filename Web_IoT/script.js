// =======================
// SMART AGRICULTURE DASHBOARD
// =======================

// ===== CONFIG =====
const channelID = "3430723";
const readAPIKey = "5SV4FJD40A79VGQY"; // isi jika channel private

const esp32IP = "http://172.20.10.3";

// ===== THINGSPEAK URL =====
const tsURL =
readAPIKey === ""

`https://api.thingspeak.com/channels/${channelID}/feeds.json?results=20`
:
`https://api.thingspeak.com/channels/${channelID}/feeds.json?api_key=${readAPIKey}&results=20`;

// =======================
// CHART SENSOR
// =======================
const ctxLine =
document.getElementById("lineDHT").getContext("2d");

const sensorChart = new Chart(ctxLine, {


type: "line",

data: {

    labels: [],

    datasets: [

        {
            label: "Suhu (°C)",
            data: [],
            borderColor: "#f97316",
            tension: 0.4
        },

        {
            label: "Humidity (%)",
            data: [],
            borderColor: "#06b6d4",
            tension: 0.4
        }
    ]
},

options: {

    responsive: true,

    maintainAspectRatio: false,

    plugins: {

        legend: {

            labels: {
                color: "white"
            }
        }
    },

    scales: {

        x: {
            ticks: {
                color: "white"
            }
        },

        y: {

            ticks: {
                color: "white"
            }
        }
    }
}


});

// =======================
// GAUGE SOIL
// =======================
const ctxGauge =
document.getElementById("gaugeSoil").getContext("2d");

const soilGauge = new Chart(ctxGauge, {

type: "doughnut",

data: {

    labels: ["Soil","Empty"],

    datasets: [{

        data: [0,100],

        backgroundColor: [
            "#22c55e",
            "#334155"
        ],

        borderWidth: 0
    }]
},

options: {

    responsive: true,

    maintainAspectRatio: false,

    circumference: 180,

    rotation: -90,

    cutout: "75%",

    plugins: {

        legend: {
            display:false
        }
    }
}


});

// =======================
// UPDATE THINGSPEAK
// =======================
async function updateThingSpeak() {


try {

    const response =
    await fetch(tsURL);

    const json =
    await response.json();

    const feeds =
    json.feeds;

    if(!feeds || feeds.length === 0) {

        console.log("Tidak ada data ThingSpeak");
        return;
    }

    const labels =
    feeds.map(feed => {

        return new Date(
            feed.created_at
        ).toLocaleTimeString();
    });

    const suhu =
    feeds.map(feed =>
        Number(feed.field1) || 0
    );

    const humidity =
    feeds.map(feed =>
        Number(feed.field2) || 0
    );

    const soil =
    Number(
        feeds[feeds.length-1].field3
    ) || 0;

    // UPDATE CARD
    document.getElementById("tempValue")
    .innerText =
    suhu[suhu.length-1] + "°C";

    document.getElementById("humValue")
    .innerText =
    humidity[humidity.length-1] + "%";

    document.getElementById("soilValue")
    .innerText =
    soil + "%";

    document.getElementById("soilTxt")
    .innerText =
    soil;

    // UPDATE CHART
    sensorChart.data.labels =
    labels;

    sensorChart.data.datasets[0].data =
    suhu;

    sensorChart.data.datasets[1].data =
    humidity;

    sensorChart.update();

    // UPDATE GAUGE
    soilGauge.data.datasets[0].data = [
        soil,
        100-soil
    ];

    if(soil < 30){

        soilGauge.data.datasets[0]
        .backgroundColor[0] =
        "#ef4444";

    }else if(soil < 60){

        soilGauge.data.datasets[0]
        .backgroundColor[0] =
        "#facc15";

    }else{

        soilGauge.data.datasets[0]
        .backgroundColor[0] =
        "#22c55e";
    }

    soilGauge.update();

    console.log("ThingSpeak Updated");

}
catch(err){

    console.error(
        "ThingSpeak Error",
        err
    );
}


}

// =======================
// STATUS ESP32
// =======================
async function updateStatus() {


try {

    const response =
    await fetch(
        esp32IP + "/status"
    );

    const data =
    await response.json();

    document.getElementById(
        "kipasStatus"
    ).innerText =
    data.kipas;

    document.getElementById(
        "pompaStatus"
    ).innerText =
    data.pompa;

}
catch(error){

    document.getElementById(
        "kipasStatus"
    ).innerText =
    "OFFLINE";

    document.getElementById(
        "pompaStatus"
    ).innerText =
    "OFFLINE";
}


}

// =======================
// CONTROL DEVICE
// =======================
async function controlDevice(
device,
action
){


try{

    await fetch(
        `${esp32IP}/${device}/${action}`
    );

    updateStatus();

}
catch(error){

    alert(
        "ESP32 Tidak Terhubung"
    );
}


}

// =======================
// AUTO REFRESH
// =======================
updateThingSpeak();
updateStatus();

setInterval(
updateThingSpeak,
15000
);

setInterval(
updateStatus,
3000
);
