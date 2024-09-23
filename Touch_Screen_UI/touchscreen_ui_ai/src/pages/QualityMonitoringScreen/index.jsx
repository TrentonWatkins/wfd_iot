import { Helmet } from "react-helmet";
import { Img, Heading } from "../../components";
import Header from "../../components/Header";
import Sidebar1 from "../../components/Sidebar1";
import WaterIntakeSwitch from "../../components/WaterIntakeSwitch";
import React, { useEffect } from "react";
import { CircularProgressbar, buildStyles } from 'react-circular-progressbar';
import 'react-circular-progressbar/dist/styles.css';

export default function QualityMonitoringScreenPage() {
  const getWheelColor = (value) => {
    if (value < 50) return 'green';
    if (value < 70) return 'orange';
    return 'red';
  };
  useEffect(() => {
    document.getElementById('header-title').innerHTML = 'Quality Monitoring Controls';
    const menuItem = document.getElementById('quality-monitoring-nav');
    if (menuItem) {
      menuItem.style.color = '#2d60ff';
    }
  }, []);

  let qualityPHLevel = 51;
  let qualityWaterLevel = 91;
  let qualityTemp = 51;
  return (
    <>
      <Helmet>
        <title>Quality Monitoring Screen</title>
        <meta name="description" content="Web site created using create-react-app" />
      </Helmet>
      <div className="w-full bg-white-a700">
        <div className="mt-[26px] flex items-start gap-[22px]">
          <Sidebar1 />
          <div className="flex flex-1 flex-col gap-3.5 self-center sm:gap-3.5">
            <Header />
           <div className="flex flex-col items-center justify-center gap-1 bg-gray-100 p-2.5 sm:gap-1">
              {/* First Row */}
              <div className="flex w-3/4 items-center sm:mr-0 gap-[10px] mb-4"> {/* Added mb-4 */}
                <WaterIntakeSwitch waterIntakeText="Water Intake Pump:" className="w-[310px] h-[170px]" />
                <Img
                  src="images/img_carret_right.svg"
                  alt="Carretright"
                  className="w-[300px] h-[150px] rounded-[40px] object-contain"
                />
                <div className="flex flex-col items-center w-[300px] gap-[10px] p-2 sm:gap-[10px] bg-white-a700 rounded-[18px]">
                  <Heading as="p" className="text-[18px] font-medium text-blue_gray-800" style={{ marginTop: '10px' }}>
                    Water Level:
                  </Heading>
                  <div className="flex items-center justify-center w-full h-full">
                    <div className="w-[120px] h-[120px]">
                      <CircularProgressbar
                        value={qualityWaterLevel}
                        text={`${qualityWaterLevel} m`}
                        styles={buildStyles({
                          pathColor: getWheelColor(qualityWaterLevel),
                          textColor: getWheelColor(qualityWaterLevel),
                        })}
                      />
                    </div>
                  </div>
                </div>
              </div>

              {/* Second Row */}
              <div className="flex w-3/4 justify-between sm:mr-0 mb-4"> {/* Added mb-4 */}
                {/* First Water Level Block */}
                <div className="flex flex-col items-center w-[300px] gap-[10px] p-2 sm:gap-[10px] bg-white-a700 rounded-[18px]">
                  <Heading as="p" className="text-[18px] font-medium text-blue_gray-800" style={{ marginTop: '10px' }}>
                    Water Temperature:
                  </Heading>
                  <div className="flex items-center justify-center w-full h-full">
                    <div className="w-[120px] h-[120px]">
                      <CircularProgressbar
                        value={qualityTemp}
                        text={`${qualityTemp} °F`}
                        styles={buildStyles({
                          pathColor: getWheelColor(qualityTemp),
                          textColor: getWheelColor(qualityTemp),
                        })}
                      />
                    </div>
                  </div>
                </div>
                {/* Second Water Level Block */}
                <div className="flex flex-col items-center w-[300px] gap-[10px] p-2 sm:gap-[10px] bg-white-a700 rounded-[18px]">
                  <Heading as="p" className="text-[18px] font-medium text-blue_gray-800" style={{ marginTop: '10px' }}>
                    Water pH Level:
                  </Heading>
                  <div className="flex items-center justify-center w-full h-full">
                    <div className="w-[120px] h-[120px]">
                      <CircularProgressbar
                        value={qualityPHLevel}
                        text={`${qualityPHLevel}`}
                        styles={buildStyles({
                          pathColor: getWheelColor(qualityPHLevel),
                          textColor: getWheelColor(qualityPHLevel),
                        })}
                      />
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </>
  );
}