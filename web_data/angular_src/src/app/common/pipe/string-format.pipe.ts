import { Pipe, PipeTransform } from '@angular/core';

@Pipe({ name: 'tooltipList' })
export class TooltipListPipe implements PipeTransform {
  public transform(lines: string[] | string): string {
    let list = '';

    if (Array.isArray(lines)) {
      lines.forEach((line) => {
        list += '• ' + line + '\n';
      });
    } else {
      list = lines;
    }

    return list;
  }
}

@Pipe({ name: 'decimalFormatPipe' })
export class DecimalFormatPipe implements PipeTransform {
  public transform(lines: string[] | string, decimals: number): string {
    let list = '';

    if (Array.isArray(lines)) {
      lines.forEach((line) => {
        const num = Number(line);

        if (!isNaN(num)) {
          list += '• ' + num.toFixed(decimals) + '\n';
        } else {
          list += '• ' + line + '\n';
        }
      });
    } else {
      const num = Number(lines);

      if (!isNaN(num)) {
        list = num.toFixed(decimals);
      } else {
        list = lines;
      }
    }

    return list;
  }
}

@Pipe({
  name: 'currencyFormatPipe',
})
export class CurrencyFormatPipe implements PipeTransform {
  public transform(value: number, decimals: number): string {
    if (value && decimals) {
      let valueString = value.toString();

      return (valueString =
        value < 0
          ? `-${this.formatNumber(value, '').toString()}`
          : this.formatNumber(value, '').toString());
    }

    return value ? value.toString() : '';
  }

  private formatNumber(number: number, prefix: string): string {
    const thousandSeparator = '.';
    const decimalSeparator = ',';
    const regex = new RegExp('[^' + decimalSeparator + '\\d]', 'g');
    const numberString = number.toString().replace(regex, '').toString();
    const split = numberString.split(decimalSeparator);
    const rest = split[0].length % 3;
    let result = split[0].substr(0, rest);
    const thousands = split[0].substr(rest).match(/\d{3}/g);

    if (thousands) {
      const separator = rest ? thousandSeparator : '';

      result += separator + thousands.join(thousandSeparator);
    }

    result =
      split[1] !== undefined ? result + decimalSeparator + split[1] : result;

    return prefix === undefined ? result : result ? prefix + result : '';
  }
}
