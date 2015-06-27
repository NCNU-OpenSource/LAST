# LAST
by 1032 Practical Linux System Administration Final Report Group 10 -  賴原群 <100321003>，梁瑜芳<101213035>

For the Graphic Version, pls go to the following link:Slide

###about LAST 
*   Light-weight
*   Adjusteable
*   Surveillance
*   Tool

###實作所需材料
| 來源     |      品名              | 價錢（NT）|
|----------|:----------------------:| ---------:|
| 課程提供 |  raspbery pi           |  //       |        
| 校園團購 |    攝影鏡頭            |  $847     |      
| 網路     | SG90 9G伺服器＊2       |  $54.8    |        
| 網路     | 公對母40條杜邦線(6used)|  $54.8    |
| 網路     | usb-to-ttl             |  $30      |        
| 網路     | 平移/傾斜伺服器支架    |  $250     |        

###使用的現有軟體與來源
|   項目       |       來源                      |
|--------------|:-------------------------------:| 
| 影像處理     | OpenCV                          |         
| 校園團購     | vlc(fot test),h264_rtspserver   |        
| 馬達控制     | ncurses(surface),pigpio         |          
| 連線及多線程 | socket,pthread                  |  

###實作過程
> 1. 影像處理
  影像處理程式直接在rpi上執行，CPU使用率達100%，有嚴重延遲現象
    於是我們將影像從rpi以串流的方式傳出，由更高效能的電腦接手處理
> 2.  旋轉圖像時邊緣會被切到
    計算旋轉後圖形框架，再將圖像平移到框架中心點
    ![rotateimg](https://raw.githubusercontent.com/NCNU-OpenSource/LAST/master/images/last-rotateimg.jpg)
> 3.  調整hsv時發生overflow
    處理邊界值
    ![rotateimg](https://github.com/NCNU-OpenSource/LAST/blob/master/images/last-hsv.png?raw=true/LAST/master/images/last-hsv.jpg)
    ![rotateimg](https://github.com/NCNU-OpenSource/LAST/blob/master/images/last-rgb.png?raw=true/LAST/master/images/last.rgb.jpg)
