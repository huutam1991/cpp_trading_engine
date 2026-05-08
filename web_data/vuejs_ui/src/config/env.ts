const isProduction = import.meta.env.PROD

let apiBaseUrl: string
let wsBaseUrl: string

if (isProduction) {
  const pageProtocol = window.location.protocol
  const pageHost = window.location.hostname
  const pagePort = window.location.port

  const wsProtocol =
    pageProtocol === 'https:' ? 'wss:' : 'ws:'

  const wsPort = pagePort
    ? Number(pagePort) + 3
    : 8443

  apiBaseUrl = window.location.origin

  wsBaseUrl =
    `${wsProtocol}//${pageHost}:${wsPort}`
}
else {
  apiBaseUrl =
    import.meta.env.VITE_API_BASE_URL

  wsBaseUrl =
    import.meta.env.VITE_WS_BASE_URL
}

export const API_BASE_URL = apiBaseUrl
export const WS_BASE_URL = wsBaseUrl