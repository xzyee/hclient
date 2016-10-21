//信息的方向：report---->Data
//从report中抽取信息，放到Data数组中（HID_DATA结构）
BOOLEAN
UnpackReport (
   _In_reads_bytes_(ReportBufferLength)PCHAR ReportBuffer,
   IN       USHORT               ReportBufferLength,
   IN       HIDP_REPORT_TYPE     ReportType,
   IN OUT   PHID_DATA            Data,
   IN       ULONG                DataLength,
   IN       PHIDP_PREPARSED_DATA Ppd
)
/*++
Routine Description:
   Given ReportBuffer representing a report from a HID device where the first
   byte of the buffer is the report ID for the report, extract all the HID_DATA
   in the Data list from the given report.
--*/
{
    ULONG       numUsages; // Number of usages returned from GetUsages.
    ULONG       i;
    UCHAR       reportID;
    ULONG       Index;
    ULONG       nextUsage;
    BOOLEAN     result = FALSE;

    reportID = ReportBuffer[0];//report的第一个字节就是report Id，这是规定

	//下面从report中解析的过程很有意思，不是从report出发，而是从data这边... 拿data去一个一个地套
  //那么data 有什么？有ReportID，这个筛了一大部分，还有usage page，只找这个usage page，还有什么？
  //collection一般不筛
  //如果是值就麻烦了，一次只能取一个值，好慢
    for (i = 0; i < DataLength; i++, Data++) 
    {
        if (reportID == Data->ReportID) 
        {
            if (Data->IsButtonData) 
            {
				        //如果Data是Button的处理
                numUsages = Data->ButtonData.MaxUsageLength;

                Data->Status = HidP_GetUsages (ReportType,
                                               Data->UsagePage,
                                               0, // All collections
                                               Data->ButtonData.Usages, //输出，UsageList
                                               &numUsages,              //输出，UsageLength
                                               Ppd,
                                               ReportBuffer, //out?
                                               ReportBufferLength);

                if (HIDP_STATUS_SUCCESS != Data->Status)
                {
                    goto Done;
                }
                
                //
                // Get usages writes the list of usages into the buffer
                // Data->ButtonData.Usages newUsage is set to the number of usages
                // written into this array.
                // A usage cannot not be defined as zero, so we'll mark a zero
                // following the list of usages to indicate the end of the list of
                // usages
                //
                // NOTE: One anomaly异常事物，反常现象 of the GetUsages function is the lack of ability
                //        to distinguish the data for one ButtonCaps from another
                //        if two different caps structures have the same UsagePage
                //        For instance:
                //          Caps1 has UsagePage 07 and UsageRange of 0x00 - 0x167
                //          Caps2 has UsagePage 07 and UsageRange of 0xe0 - 0xe7
                //
                //        However, calling GetUsages for each of the data structs
                //          will return the same list of usages.  It is the 
                //          responsibility of the caller to set in the HID_DEVICE
                //          structure which usages actually are valid for the
                //          that structure. 
                //      

                /*
                // Search through the usage list and remove those that 
                //    correspond to usages outside the define ranged for this
                //    data structure.
                */
                
                for (Index = 0, nextUsage = 0; Index < numUsages; Index++) 
                {
                    if (Data -> ButtonData.UsageMin <= Data -> ButtonData.Usages[Index] &&
                            Data -> ButtonData.Usages[Index] <= Data -> ButtonData.UsageMax) 
                    {
                        Data -> ButtonData.Usages[nextUsage++] = Data -> ButtonData.Usages[Index];
                    }
                }

                if (nextUsage < Data -> ButtonData.MaxUsageLength) 
                {
                    Data->ButtonData.Usages[nextUsage] = 0;
                }
            }
            else 
            {
				        //如果Data是value的处理
                Data->Status = HidP_GetUsageValue (
                                                ReportType,
                                                Data->UsagePage,
                                                0,               // All Collections.
                                                Data->ValueData.Usage,
                                                &Data->ValueData.Value, //只有这个是输出
                                                Ppd,
                                                ReportBuffer,
                                                ReportBufferLength);

                if (HIDP_STATUS_SUCCESS != Data->Status)
                {
                    goto Done;
                }
				      //值要一个一个地取，好慢，一次只能取一个usage的
                Data->Status = HidP_GetScaledUsageValue (
                                                       ReportType,
                                                       Data->UsagePage,
                                                       0, // All Collections.
                                                       Data->ValueData.Usage,
                                                       &Data->ValueData.ScaledValue,//只有这个是输出
                                                       Ppd,
                                                       ReportBuffer,
                                                       ReportBufferLength);

				//返回HIDP_STATUS_SUCCESS或者HIDP_STATUS_NULL都认为是成功
				//如果失败则直接退出程序
                if (HIDP_STATUS_SUCCESS != Data->Status &&
                    HIDP_STATUS_NULL != Data->Status)
                {
                    goto Done;
                }

            } 
            Data -> IsDataSet = TRUE;
        }
    }

    result = TRUE;

Done:
    return (result);
}

