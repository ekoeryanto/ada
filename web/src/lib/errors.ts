// Error handling utilities for Astro components
export class AppError extends Error {
  constructor(
    message: string,
    public code?: string,
    public statusCode?: number,
    public context?: Record<string, any>
  ) {
    super(message);
    this.name = 'AppError';
  }
}

export class APIError extends AppError {
  constructor(
    message: string,
    statusCode: number,
    endpoint?: string,
    response?: any
  ) {
    super(message, 'API_ERROR', statusCode, { endpoint, response });
    this.name = 'APIError';
  }
}

export function handleError(error: unknown): AppError {
  if (error instanceof AppError) {
    return error;
  }

  if (error instanceof Error) {
    return new AppError(error.message, 'UNKNOWN_ERROR', undefined, {
      originalError: error,
    });
  }

  return new AppError(
    'An unknown error occurred',
    'UNKNOWN_ERROR',
    undefined,
    { originalError: error }
  );
}

// Global error handler for unhandled promises
export function setupGlobalErrorHandling() {
  window.addEventListener('unhandledrejection', (event) => {
    console.error('Unhandled promise rejection:', event.reason);
    
    // Add to store notifications if available
    if ((window as any).adaActions?.addNotification) {
      (window as any).adaActions.addNotification({
        type: 'error',
        title: 'Unexpected Error',
        message: event.reason?.message || 'An unexpected error occurred',
      });
    }
  });

  window.addEventListener('error', (event) => {
    console.error('Global error:', event.error);
    
    // Add to store notifications if available
    if ((window as any).adaActions?.addNotification) {
      (window as any).adaActions.addNotification({
        type: 'error',
        title: 'Application Error',
        message: event.error?.message || 'An application error occurred',
      });
    }
  });
}

// Retry utility for failed operations
export async function retryOperation<T>(
  operation: () => Promise<T>,
  maxRetries: number = 3,
  delay: number = 1000
): Promise<T> {
  let lastError: Error;
  
  for (let attempt = 1; attempt <= maxRetries; attempt++) {
    try {
      return await operation();
    } catch (error) {
      lastError = error instanceof Error ? error : new Error(String(error));
      
      if (attempt === maxRetries) {
        throw lastError;
      }
      
      // Exponential backoff
      await new Promise(resolve => setTimeout(resolve, delay * attempt));
    }
  }
  
  throw lastError!;
}