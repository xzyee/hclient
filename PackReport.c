BOOLEAN
PackReport (
   _Out_writes_bytes_(ReportBufferLength)PCHAR ReportBuffer,
   IN  USHORT               ReportBufferLength,
   IN  HIDP_REPORT_TYPE     ReportType,
   IN  PHID_DATA            Data,
   IN  ULONG                DataLength,
   IN  PHIDP_PREPARSED_DATA Ppd
   )
/*++
   用很多HID_DATA结构的表来合成一个report,里面ID不一样怎么办？用第一项里面的report ID
     就会有一些HID_DATA结构的数据被处理，另一些没有被处理，被处理的要做个标记（IsDataSet =TRUE）
--*/
{
    ULONG       numUsages; // Number of usages to set for a given report.
    ULONG       i;
    ULONG       CurrReportID;
    BOOLEAN     result = FALSE;
    //report首先必须被清零
    memset (ReportBuffer, (UCHAR) 0, ReportBufferLength);
    //使用第一个HID_DATA的数据作为ID，被处理的ID必须与之相同
    CurrReportID = Data -> ReportID;
    for (i = 0; i < DataLength; i++, Data++)  //遍历所有的Data，并不是所有的Data都一定会得到处理
    {
        /*
        // There are two different ways to determine if we set the current data
        //    structure:
        //    1) Store the report ID were using and only attempt to set those
        //        data structures that correspond to the given report ID.  This
        //        example shows this implementation.
        //
        //    2) Attempt to set all of the data structures and look for the
        //        returned status value of HIDP_STATUS_INVALID_REPORT_ID.  This
        //        error code indicates that the given usage exists but has a
        //        different report ID than the report ID in the current report
        //        buffer
        */
        if (Data -> ReportID == CurrReportID) //ID必须相同的Data才能合并到一个report中
        {
            if (Data->IsButtonData)
            {   //button的处理，能处理好几个，因为button只占1bit
                numUsages = Data->ButtonData.MaxUsageLength;
                Data->Status = HidP_SetUsages (ReportType,
                                               Data->UsagePage,
                                               0, // All collections
                                               Data->ButtonData.Usages,//buttons所有的usage，不只是一个
                                               &numUsages, //buttons的个数
                                               Ppd,
                                               ReportBuffer, //合并到这个报告中
                                               ReportBufferLength);
            }
            else
            {   //value的处理,只有一个值
                Data->Status = HidP_SetUsageValue (ReportType,
                                                   Data->UsagePage,
                                                   0, // All Collections.
                                                   Data->ValueData.Usage,
                                                   Data->ValueData.Value,
                                                   Ppd,
                                                   ReportBuffer,
                                                   ReportBufferLength);
            }
            if (HIDP_STATUS_SUCCESS != Data->Status)
            {
                goto Done;
            }
        }
    }  
    /*
    // At this point, all data structures that have the same ReportID as the
    //    first one will have been set in the given report.  Time to loop
    //    through the structure again and mark all of those data structures as
    //    having been set.
    */
    for (i = 0; i < DataLength; i++, Data++)
    {
        if (CurrReportID == Data -> ReportID)
        {
            Data -> IsDataSet = TRUE;
        }
    }
    result = TRUE;
Done:
    return result;
}
