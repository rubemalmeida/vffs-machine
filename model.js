document.addEventListener('DOMContentLoaded', function () {
    var $delay = 3000,
        vMin = 11.5,
        vMax = 14.5,
        cMin = .3,
        cMax = 2.5,
        mMin = 0,
        mMax = 5,
        totalPoints = 25,
        descVazao = 'Vazão',
        descVolume = 'Volume',
        descEnvase = 'Envase',
        vazaoDisplay = document.getElementById('vazao'),
        volumeDisplay = document.getElementById('volume'),
        envaseDisplay = document.getElementById('envase');

    function addEventToFeed(event) {
        // if no event, return
        if (!event) return;
        const feed = document.getElementById('event-feed');
        const li = document.createElement('li');
        li.textContent = event;
        console.log(event);

        // Adiciona classe para fade in
        li.classList.add('fade-in');

        // Verifica se há filhos na lista
        if (feed.firstChild) {
            // Adiciona o novo evento no topo da lista
            feed.insertBefore(li, feed.firstChild);
        } else {
            // Se não houver filhos, simplesmente adiciona à lista
            feed.appendChild(li);
        }

        // Remove classe de fade in após um pequeno intervalo
        setTimeout(() => {
            li.classList.remove('fade-in');

            // Adiciona classe para fade out
            li.classList.add('fade-out');

            // Remove o evento após 5 segundos
            setTimeout(() => {
                li.remove();
            }, 5000);
        }, 100);
    }

    function processLogs(index) {
        if (index < logs.length) {
            addEventToFeed(logs[index]);

            // Remove o evento do array
            logs.splice(index, 1);

            // Atraso de 500ms antes da próxima iteração
            setTimeout(() => {
                processLogs(index);
            }, 500);
        } else {
            // aguarda 1s e reinicia o processamento
            setTimeout(() => {
                processLogs(0);
            }, 1000);
        }
    }

    function getRandomInt(min, max) {
        let reading = (Math.random() * (max - min + 1) + min);
        return (Math.round(reading * 2) / 2)
    }

    function updateVazao(value) {
        vazaoDisplay.innerHTML = parseFloat(value).toFixed(1) + '<span>ml/s</span>';
    }

    function updateVolume(value) {
        volumeDisplay.innerHTML = parseFloat(value).toFixed(1) + '<span>ml</span>';
    }

    function updateEnvase(value) {
        envaseDisplay.innerHTML = parseFloat(value).toFixed(0) + '<span>un</span>';
    }

    function updateSensorDisplayValues(d) {
        updateVazao(d[0]);
        updateVolume(d[1]);
        updateEnvase(d[2]);
    }

    Highcharts.setOptions({
        global: {
            useUTC: false
        },
        plotOptions: {
            series: {
                marker: {
                    enabled: false
                }
            }
        },
        tooltip: {
            enabled: false
        }
    });

    Highcharts.chart("sensorData", {
        chart: {
            type: 'spline',
            events: {
                load: function () {
                    var yAxisVazao = this.yAxis[0];
                    var yAxisVolume = this.yAxis[1];
                    var serieVazao = this.series[0];
                    var serieVolume = this.series[1];
                    var x, vazao, volume, envase;

                    // faking sensor data
                    // data will be coming from sensors on the MKR1000
                    setInterval(function () {

                        yAxisVazao.update({ max: globalVazao * 1.1 });
                        yAxisVolume.update({ max: globalVolume * 1.1 });

                        x = (new Date()).getTime(),
                            vazao = globalVazao,
                            volume = globalVolume,
                            envase = globalEnvase;
                        // vazao = getRandomInt(vMin, vMax),
                        // volume = getRandomInt(cMin, cMax),
                        // envase = getRandomInt(mMin, mMax);

                        serieVazao.addPoint([x, vazao], false, true);
                        serieVolume.addPoint([x, volume], true, true);

                        updateSensorDisplayValues([vazao, volume, envase]);
                    }, $delay);
                }
            }
        },
        title: {
            text: 'Sensor Data'
        },
        xAxis: {
            type: 'datetime',
            tickPixelInterval: 500
        },
        yAxis: [{
            title: {
                text: descVazao.toUpperCase(),
                style: {
                    color: '#2b908f',
                    font: '13px sans-serif'
                }
            },
            min: 0,
            max: 100,
            plotLines: [{
                value: 0,
                width: 1,
                color: '#808080'
            }]
        }, {
            title: {
                text: descVolume.toUpperCase(),
                style: {
                    color: '#90ee7e',
                    font: '13px sans-serif'
                }
            },
            min: 0,
            max: globalVolumePrevisto * 1.5,
            opposite: true,
            plotLines: [{
                value: 0,
                width: 1,
                color: '#808080'
            }]
        }],
        tooltip: {
            formatter: function () {
                var unitOfMeasurement = "";
                if (this.series.name === 'VAZÃO') {
                    unitOfMeasurement = ' ml/s';
                } else if (this.series.name === 'VOLUME') {
                    unitOfMeasurement = ' ml';
                } else if (this.series.name === 'ENVASE') {
                    unitOfMeasurement = ' un';
                }
                return '<b>' + this.series.name + '</b><br/>' +
                    Highcharts.numberFormat(this.y, 1) + unitOfMeasurement;
            }
        },
        legend: {
            enabled: true
        },
        exporting: {
            enabled: false
        },
        series: [{
            name: descVazao.toUpperCase(),
            yAxis: 0,
            style: {
                color: '#2b908f'
            },
            data: (function () {
                // generate an array of random data
                var data = [],
                    time = (new Date()).getTime(),
                    i;

                for (i = -totalPoints; i <= 0; i += 1) {
                    data.push({
                        x: time + i * $delay,
                        y: 0
                        //y: getRandomInt(12, 12)
                    });
                }
                return data;
            }())
        }, {
            name: descVolume.toUpperCase(),
            yAxis: 1,
            data: (function () {
                // generate an array of random data
                var data = [],
                    time = (new Date()).getTime(),
                    i;

                for (i = -totalPoints; i <= 0; i += 1) {
                    data.push({
                        x: time + i * $delay,
                        y: 0
                        //y: getRandomInt(.1, .7)
                    });
                }
                return data;
            }())
        }]
    });

    processLogs(0);
});
