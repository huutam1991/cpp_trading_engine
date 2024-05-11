import * as _ from 'lodash';
import * as moment from 'moment';
import { BehaviorSubject, Observable } from 'rxjs';

export class DataTableUtils {
  public static endTimeISO = 'T23:59:59.999Z';

  public static getValueByKey(element: any, keyIn: string): any {
    if (!element) {
      return null;
    }

    // split by +
    const multiKeyArr: Array<any> = keyIn.split('+');
    const finalValue: Array<any> = [];

    multiKeyArr.forEach((eachKey) => {
      // split the key

      const keyArr: Array<any> = this.splitFirstOccur(eachKey, '.');
      // get first

      if (!keyArr) {
        finalValue.push(null);
      }

      if (keyArr.length === 2) {
        // have multiple
        const key = keyArr[0];
        const nextKey = keyArr[1];

        const value = this.processElementKey(element, key, nextKey);

        finalValue.push(value);
      } else if (keyArr.length === 1) {
        // only one left
        const key = keyArr[0];
        const value = this.processElementKey(element, key, null);

        finalValue.push(value);
      } else {
        finalValue.push(null);
      }
    });

    // post recursive processing
    if (finalValue.length <= 1) {
      const combineArr = finalValue[0];
      let filteredValue;

      if (Array.isArray(combineArr)) {
        filteredValue = combineArr.filter((el) => el !== null);
      } else {
        filteredValue = combineArr;
      }

      return filteredValue;
    } else if (finalValue.length === 2) {
      // supporting up to 2 for now
      // split and if only one then merge them
      const array1: any = finalValue[0];
      const array2: any = finalValue[1];

      if (!Array.isArray(array1) && !Array.isArray(array2)) {
        return array1 && array2
          ? array1 + ': ' + array2
          : array1 !== null
          ? array1
          : null;
      } else {
        const combineArr = array1.map((e: any, i: any) => {
          if (array2[i]) {
            // need to fix this shit up
            return e + ': ' + array2[i];
          } else {
            return e;
          }
        });

        return combineArr.filter((el: any) => el !== null);
      }
    }

    return finalValue.filter((el) => el !== null);
  }

  public static processElementKey(
    element: any,
    key: any,
    nextKey: any
  ): string | null {
    if (key.slice(-1) === ']') {
      // is array slice it apart
      const splitKey = key.split('[');

      const keyId = splitKey[0];
      const arrNumber = splitKey[1].slice(0, -1);

      if (arrNumber === 'all') {
        // loop and print all
        const value: any = [];

        if (!element[keyId]) {
          return null;
        }

        element[keyId].forEach((el: any) => {
          value.push(nextKey ? this.getValueByKey(el, nextKey) : el);
        });

        return value;
      } else {
        return nextKey
          ? this.getValueByKey(
              element[keyId] ? element[keyId][arrNumber] : null,
              nextKey
            )
          : element[keyId]
          ? element[keyId][arrNumber]
          : null;
      }
    } else {
      return nextKey ? this.getValueByKey(element[key], nextKey) : element[key];
    }
  }

  public static splitFirstOccur(str: any, splitBy: any): any[] {
    const keyArr = [];
    const first = str.substr(0, str.indexOf(splitBy));

    if (first) {
      keyArr.push(first);
    }

    const second = str.substr(str.indexOf(splitBy) + 1);

    if (second) {
      keyArr.push(second);
    }

    return keyArr;
  }

  public static isString(element: any): boolean {
    if (typeof element === 'string' || element instanceof String) {
      return true;
    } else {
      return false;
    }
  }

  public static isDate(input: any): boolean {
    if (Object.prototype.toString.call(input) === '[object Date]') {
      return true;
    }

    return false;
  }

  public static isFormEmpty(editPageFields: any): boolean {
    const addIsEdited = editPageFields.find((field: any) => {
      if (field.hidden !== undefined && field.hidden) {
        return false;
      }

      if (field.type === 'blank') {
        return false;
      }

      if (field.type === 'slidetoggle') {
        return field.formControl.value === true ? false : true;
      } else if (field.type === 'phoneNumber') {
        return (field.formControl.controls[0].value &&
          field.formControl.controls[0].value.length > 0) ||
          (field.formControl.controls[1].value &&
            field.formControl.controls[1].value.length > 0)
          ? true
          : false;
      } else if (field.type === 'salutationName') {
        return (field.formControl.value &&
          field.formControl.value.length > 0) ||
          (field.formControlName.value &&
            field.formControlName.value.length > 0)
          ? true
          : false;
      } else if (field.type === 'groupFields') {
        return false;
      } else {
        if (
          typeof field.formControl.value !== 'string' &&
          _.isArray(field.formControl.value)
        ) {
          const stringified = JSON.stringify(field.formControl.value);

          return stringified && stringified.length > 0 ? true : false;
        }

        return field.formControl.value && field.formControl.value.length > 0
          ? true
          : false;
      }
    });

    if (addIsEdited) {
      return true;
    }

    return false;
  }

  public static isFormEdited(data: any, editPageFields: any): boolean {
    const editIsEdited = editPageFields.find((field: any) => {
      if (field.hidden !== undefined && field.hidden) {
        return false;
      }

      if (field.type === 'blank') {
        return false;
      }

      if (field.type === 'slidetoggle') {
        if (field.getBooleanValue !== undefined && field.getBooleanValue) {
          return this.getValueByKey(data, field.id) !== field.formControl.value
            ? true
            : false;
        } else {
          return this.getValueByKey(data, field.id) !==
            (field.formControl.value === null
              ? 'pending'
              : field.formControl.value
              ? 'enabled'
              : 'disabled')
            ? true
            : false;
        }
      } else if (field.type === 'phoneNumber') {
        return this.getValueByKey(data, field.id) !==
          `(${field.formControl.controls[0].value})${field.formControl.controls[1].value}`
          ? true
          : false;
      } else if (field.type === 'salutationName') {
        return this.getValueByKey(data, field.id) !==
          field.formControl.controls[0].value &&
          this.getValueByKey(data, field.subId) !==
            field.formControl.controls[1].value
          ? true
          : false;
      } else if (field.type === 'groupFields') {
        return false;
      } else {
        const retVal = this.getValueByKey(data, field.id);

        if (retVal !== null && this.isJsonString(_.cloneDeep(retVal))) {
          return !_.isEqual(JSON.parse(retVal), field.formControl.value)
            ? true
            : false;
        } else {
          return !_.isEqual(retVal, field.formControl.value) ? true : false;
        }
      }
    });

    if (editIsEdited) {
      return true;
    }

    return false;
  }

  public static isJsonString(str: any): boolean {
    if (typeof str === 'string') {
      if (_.includes(str, '[') || _.includes(str, '{')) {
        try {
          JSON.parse(str);

          return true;
        } catch (e) {
          return false;
        }
      }
    }

    return false;
  }

  public static transformText(text: any): string {
    return text.toLowerCase().replace(/[^a-z0-9]+/g, '');
  }

  public static getDataGroup(editPageFields: any): any[] {
    let dataGroupArr: any[] = [];

    _.forEach(editPageFields, (field: any) => {
      if (field.dataGroup) {
        dataGroupArr.push(field.dataGroup);
      }
    });

    if (dataGroupArr.length > 0) {
      dataGroupArr = _.union(dataGroupArr);
    }

    return dataGroupArr;
  }

  public static convertObservableToBehaviorSubject<T>(
    observable: Observable<T>,
    initValue: T
  ): BehaviorSubject<T> {
    const subject = new BehaviorSubject(initValue);

    observable.subscribe(
      (x: T) => {
        subject.next(x);
      },
      (err: any) => {
        subject.error(err);
      },
      () => {
        subject.complete();
      }
    );

    return subject;
  }

  public static pickObjectKeysFromArray(array: any, thingsToPick: any): any {
    return _.map(
      array,
      _.partial(_.ary(_.pick, thingsToPick.length), _, thingsToPick)
    );
  }

  public static mapToISOString(date: any): string | null {
    if (!date) {
      return null;
    }

    if (typeof date === 'string' && date.indexOf('T') !== -1) {
      return date;
    }

    date = moment(date, 'DD/MM/YYYY').toDate();

    const currentMonth = 1 + date.getMonth();
    const month = this.addZero(currentMonth);
    const day = this.addZero(date.getDate());
    const h = this.addZero(date.getHours());
    const m = this.addZero(date.getMinutes());
    const s = this.addZero(date.getSeconds());

    return (
      date.getFullYear() +
      '-' +
      month +
      '-' +
      day +
      'T' +
      [h, m, s].join(':') +
      '.000Z'
    );
  }

  private static addZero(i: any): string {
    if (i < 10) {
      i = '0' + i;
    }

    return i;
  }

  public static validDate(date: any): boolean {
    if (moment(date, 'DD/MM/YYYY', true).isValid()) {
      date = date.split('/');
      const dd = +date[0];
      const mm = +date[1];
      const yy = +date[2];
      const listofDays = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];

      if (mm === 1 || mm > 2) {
        if (dd > listofDays[mm - 1]) {
          new Error('Invalid date format!');

          return false;
        }
      }

      if (mm === 2) {
        let leapYear = false;

        if ((!(yy % 4) && yy % 100) || !(yy % 400)) {
          leapYear = true;
        }

        if (leapYear === false && dd >= 29) {
          new Error('Invalid date format!');

          return false;
        }

        if (leapYear === true && dd > 29) {
          new Error('Invalid date format!');

          return false;
        }
      }

      return true;
    } else {
      return false;
    }
  }

  public static getPreviousYear(): string {
    return new Date(
      new Date().setFullYear(new Date().getFullYear() - 100)
    ).toISOString();
  }

  public static getFutureYear(): string {
    return new Date(
      new Date().setFullYear(new Date().getFullYear() + 100)
    ).toISOString();
  }

  public static validPhoneNumber(value: string): boolean {
    return /^([0-9/-]*)$/g.test(value);
  }

  public static convertDateFormat(
    inputDate: Date,
    format: 'YYYYMMDD' | 'YYYYMMDDHHMM' | 'YYYYMMDDHHMMSS' | 'YYYYMMDDHHMMSSMS',
    separator: string = '-'
  ): string {
    const year = inputDate.getFullYear();
    const month = inputDate.getMonth() + 1;
    const date = inputDate.getDate();
    const hours = inputDate.getHours();
    const minutes = inputDate.getMinutes();
    const seconds = inputDate.getSeconds();
    const miliSeconds = inputDate.getMilliseconds();

    switch (format) {
      case 'YYYYMMDD':
        return [year, month, date].join(separator);

      case 'YYYYMMDDHHMM':
        return (
          [year, month, date].join(separator) + ' ' + [hours, minutes].join(':')
        );

      case 'YYYYMMDDHHMMSS':
        return (
          [year, month, date].join(separator) +
          ' ' +
          [hours, minutes, seconds].join(':')
        );

      case 'YYYYMMDDHHMMSSMS':
        return (
          [year, month, date].join(separator) +
          ' ' +
          [hours, minutes, seconds].join(':') +
          '.' +
          miliSeconds
        );

      default:
        return inputDate.toString();
    }
  }
}

export const getBase64 = (file: File): Promise<string | ArrayBuffer | null> =>
  new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.readAsDataURL(file);
    reader.onload = () => resolve(reader.result);
    reader.onerror = (error) => reject(error);
  });

export const getColorForStatus = (status: number): string => {
  switch (status) {
    case 1:
      return '#047857';
    case 2:
      return '#60a5fa';
    case 3:
      return '#f59e0b';
    case 4:
      return '#171717';
    case 5:
      return '#ef4444';
    case 6:
      return '#86efac';
    default:
      return '#22d3ee';
  }
};
