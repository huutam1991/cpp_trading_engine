import { Pipe, PipeTransform } from '@angular/core';

import * as moment from 'moment';

@Pipe({ name: 'datetimeformat' })
export class DateTimeformatPipe implements PipeTransform {
  public transform(value: number, dtFormat: string): string {
    let result = '';

    if (value && value !== undefined && value !== null) {
      let timeVal: number = value;

      if (String(value).length > 10) {
        timeVal = value / 1000;
      }

      if (dtFormat && dtFormat.length > 0) {
        result = moment.unix(timeVal).format(dtFormat);
      }
    }

    return result;
  }
}

@Pipe({ name: 'datetimeformat2' })
export class DateTimeformat2Pipe implements PipeTransform {
  private formates: Array<string> = [
    'MM-DD-YYYY',
    'DD-MM-YYYY',
    'hmmss',
    'YYYY-MM-DD',
    'DD-MM-YYYY h:mm A',
    'YYYY-MM-DD h:mm A',
    'YYYY-MM-DDTHH:mm',
    'YYYY-MM-DDTHH:mm:ss',
    'HH:mm', 'HH:mm:ss'
  ];
  public transform(value: string, dtFormat: string): string {
    let result = '';

    if (value && value !== undefined && value !== null) {
      if (dtFormat && dtFormat.length > 0) {
        result = moment(value, this.formates).format(dtFormat);
      }
    }

    return result;
  }
}

@Pipe({ name: 'datetimeformat3' })
export class DateTimeformat3Pipe implements PipeTransform {
  public transform(value: string, dtFormat: string): string {
    let result = '';

    if (value && value !== undefined && value !== null) {
      if (dtFormat && dtFormat.length > 0) {
        result = moment(value, 'YYYY-MM-DD h:mm A').format(dtFormat);
      }
    }

    return result;
  }
}

@Pipe({ name: 'datetimeUtcFormat' })
export class DateTimeUtcFormat implements PipeTransform {
  public transform(value: string, dtFormat: string): string {
    let result = '';

    if (value && value !== undefined && value !== null) {
      if (dtFormat && dtFormat.length > 0) {
        result = moment.utc(value).format(dtFormat);
      }
    }

    return result;
  }
}

@Pipe({ name: 'datetimeUtc2LocalFormat' })
export class DateTimeUtc2LocalFormat implements PipeTransform {
  public transform(value: string, dtFormat: string): string {
    let result = '';

    if (value && value !== undefined && value !== null) {
      if (dtFormat && dtFormat.length > 0) {
        const utcTime = moment.utc(value).toDate();

        result = moment(utcTime).local().format(dtFormat);
      }
    }

    return result;
  }
}

