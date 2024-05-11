import { MatPaginatorIntl } from '@angular/material/paginator';

const viRangeLabel = (
  page: number,
  pageSize: number,
  length: number
): string => {
  if (length === 0 || pageSize === 0) {
    return `0-0 trong số ${length} bản ghi`;
  }

  length = Math.max(length, 0);

  const startIndex = page * pageSize;

  // If the start index exceeds the list length, do not try and fix the end index to the end.
  const endIndex =
    startIndex < length
      ? Math.min(startIndex + pageSize, length)
      : startIndex + pageSize;

  return `${startIndex + 1} - ${endIndex} trong số ${length} bản ghi`;
};

export const getVietNamPaginatorIntl = (): MatPaginatorIntl => {
  const paginatorIntl = new MatPaginatorIntl();

  paginatorIntl.itemsPerPageLabel = 'Bản ghi/trang';
  paginatorIntl.getRangeLabel = viRangeLabel;
  paginatorIntl.firstPageLabel = 'Trang đầu tiên';
  paginatorIntl.previousPageLabel = 'Trang trước';
  paginatorIntl.nextPageLabel = 'Trang kế tiếp';
  paginatorIntl.lastPageLabel = 'Trang cuối';

  return paginatorIntl;
};
